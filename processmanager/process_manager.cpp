#include <signal.h>
#include <errno.h>
#include <string.h>
#include <poll.h>
#include <sys/wait.h>
#include <map>
#include <memory>
#include "fd_watcher.h"
#include "signal_watcher.h"
#include "process_manager.h"
#include "process.h"
#include "framework_log.h"
#include "framework_utils.h"


using namespace std;


// log tag for this file
#define TAG "ProcessManager"

// wait child processes terminated timeout (unit: ms)
#define WAIT_TERM_TIMEOUT 300


namespace {

// all running sub processes
map<pid_t, std::shared_ptr<Process>>  processes;

// the mutex lock for processes
Mutex processesMutex;


}; // anonymous namespace


pid_t ProcessManager::create(const string& file, const vector<string>& args) {
    // check file exist
    if (!pathExist(file)) {
        LOGE(TAG, "can't create process: %s is not existed", file.c_str());
        return -1;
    }
    
    // try to start a new process
    shared_ptr<Process> process = make_shared<Process>();
    if (!process->start(file, args, "", "", "")) {
        LOGE(TAG, "can't create process for %s", file.c_str());
        return -1;
    }
    

    // add the child to process list
    Lock lock(processesMutex);
    processes[process->id()] = process;

    // add the child process' stdin/stdout/stderr to fd wathcer
    FDWatcher& fdWatcher =  FDWatcher::instance();
    fdWatcher.add(FDWatcher::FD_WRITE, process->inFD());
    fdWatcher.add(FDWatcher::FD_READ, process->outFD());
    fdWatcher.add(FDWatcher::FD_READ, process->errFD());
    process->dump();
    return process->id();
}


pid_t ProcessManager::create(const std::string& file, const std::vector<std::string>& args, 
    const std::string& stderrFile) {

    // check file exist
   if (!pathExist(file)) {
       LOGE(TAG, "can't create process: %s is not existed", file.c_str());
       return -1;
   }

   // check stderr file name
   if (stderrFile.empty()) {
       LOGE(TAG, "can't create process: stderr file name is empty");
       return -1;
   }
   
   
   // try to start a new process
   shared_ptr<Process> process = make_shared<Process>();
   if (!process->start(file, args, "", "", stderrFile)) {
       LOGE(TAG, "can't create process for %s", file.c_str());
       return -1;
   }
   

   // add the child to process list
   Lock lock(processesMutex);
   processes[process->id()] = process;

   // add the child process' stdin to fd wathcer
   FDWatcher& fdWatcher =  FDWatcher::instance();
   fdWatcher.add(FDWatcher::FD_WRITE, process->inFD());
   process->dump();
   return process->id();

    
}


pid_t ProcessManager::create(const std::string& file, const std::vector<std::string>& args, 
    const std::string& stdoutFile, const std::string& stderrFile) {
              
    // check file exist
    if (!pathExist(file)) {
        LOGE(TAG, "can't create process: %s is not existed", file.c_str());
        return -1;
    }

    // check stdout/stderr file name
    if (stdoutFile.empty() || stderrFile.empty()) {
        LOGE(TAG, "can't create process: stdout file name and/or stderr file name is empty");
        return -1;
    }
    
    // try to start a new process
    shared_ptr<Process> process = make_shared<Process>();
    if (!process->start(file, args, "", stdoutFile, stderrFile)) {
        LOGE(TAG, "can't create process for %s", file.c_str());
        return -1;
    }

    // add the child to process list
    Lock lock(processesMutex);
    processes[process->id()] = process;

    // add the child process' stdin to fd wathcer
    FDWatcher& fdWatcher =  FDWatcher::instance();
    fdWatcher.add(FDWatcher::FD_WRITE, process->inFD());
    process->dump();
    return process->id();
}




bool ProcessManager::terminate(pid_t pid) {
    Lock lock(processesMutex);
    if (processes.find(pid) != processes.end()) {
        LOGD(TAG, "terminate(): process %d is not existed.", pid);
        return false;
    }
    
    // send SIGTERM to the child
    int err = ::kill(pid, SIGTERM);
    if (err) {
        LOGE(TAG, "kill process %d with SIGTERM failed: %s", pid, strerror(errno));
        return false;
    }
    return true;
}


bool ProcessManager::kill(pid_t pid) {
    Lock lock(processesMutex);
    if (processes.find(pid) != processes.end()) {
        LOGD(TAG, "kill(): process %d is not existed.", pid);
        return false;
    }
    
    // send SIGKILL to the child
    int err = ::kill(pid, SIGKILL);
    if (err) {
        LOGE(TAG, "kill process %d with SIGKILL failed: %s", pid, strerror(errno));
        return false;
    }
    return true;
}


bool ProcessManager::isAlive(pid_t pid) {
    Lock lock(processesMutex);
    if (processes.find(pid) == processes.end()) {
        LOGD(TAG, "isAlive(): process %d is not existed.", pid);
        return false;
    }
    return probeAlive(pid);
}

int ProcessManager::inFD(pid_t pid) {
    Lock lock(processesMutex);
    if (processes.find(pid) == processes.end()) {
        LOGD(TAG, "inFD(): process %d is not existed.", pid);
        return -1;
    }
    return processes[pid]->inFD();
}

int ProcessManager::outFD(pid_t pid) {
    Lock lock(processesMutex);
    if (processes.find(pid) == processes.end()) {
        LOGD(TAG, "outFD(): process %d is not existed.", pid);
        return -1;
    }   
    return processes[pid]->outFD();
}

int ProcessManager::errFD(pid_t pid) {
    Lock lock(processesMutex);
    if (processes.find(pid) == processes.end()) {
        LOGD(TAG, "errFD(): process %d is not existed.", pid);
        return -1;
    }
    return processes[pid]->errFD();
}

ssize_t ProcessManager::recvData(pid_t pid, void* buf, size_t buflen, int timeout) {
    // got process' stdout
    int fd = outFD(pid);
    if (fd == -1) {
        return -1;
    }

    // read process' stdout
    ssize_t ret = read(fd, buf, buflen, timeout);

    // read error
    if (ret == -1) {
        LOGD(TAG, "read process %d stdout fd %d error", pid, fd);
        Event event;
        event.type = FD_READ_ERR;
        event.data.fd = fd;
        notify(event);
        return ret;
    }

    // read timeout (or end of file)
    if (ret == 0) {
        //LOGD(TAG, "poll process %d stdout fd %d timeout (or end of file)", pid, fd);
        return ret;
    }

    // read succeed
    return ret;
} 


ssize_t ProcessManager::sendData(pid_t pid, const void* buf, size_t buflen, int timeout) {
    // got process stdin
    int fd = inFD(pid);
    if (fd == -1) {
        return -1;
    }

    // write process' stdin
    ssize_t ret = write(fd, buf, buflen, timeout);

    // write error
    if (ret == -1) {
        LOGD(TAG, "write process %d stdin fd %d error", pid, fd);
        Event event;
        event.type = FD_WRITE_ERR;
        event.data.fd = fd;
        notify(event);
        return ret;
    }

    // write timeout (or nothing was written)
    if (ret == 0) {
        // LOGD(TAG, "poll process %d stdin fd %d timeout (or nothing was written)", pid, fd);
        return ret;
    }

    // write succeed
    return ret;
}

void ProcessManager::notify(const Event& event) {
    Lock lock(processesMutex);
    switch (event.type) {
        case FD_READ_ERR:
        case FD_WRITE_ERR:
        case FD_POLL_ERR: {
            LOGD(TAG, "notify: type=%d, fd=%d", event.type, event.data.fd);
            pid_t pid = owner(event.data.fd);
            if (!probeAlive(pid)) {
                recycle(pid);
            }
        }
            break;

        case SIG_CHLD_EXIT: {
            LOGD(TAG, "notify: type=%d, pid=%d", event.type, event.data.pid);
            recycle(event.data.pid);
        }
            break;    
            
        default:
            LOGD(TAG, "notify: unknown type=%d", event.type);
            break;
    }
}


bool ProcessManager::start() {
    if (!SignalWatcher::instance().start()) {
        return false;
    }
        
    if (!FDWatcher::instance().start()) {
        return false;
    }
    
    return true;
}

bool ProcessManager::stop() {
    bool status = true;
    
    // try to terminate all child processes
    {Lock lock(processesMutex);
        terminateAll();
    }

    // wait child processess terminated 
    usleep(WAIT_TERM_TIMEOUT*300);

    // kill all child processess if still alive
    {Lock lock(processesMutex);
        if (!processes.empty()) {
            status = terminateAll();
        }
    }

    // stop fd wather
    if (!FDWatcher::instance().stop()) {
        status = false;
    }
    // stop signal watcher
    if (!SignalWatcher::instance().stop()) {
        status = false;
    }
    return status;
}


//
// NOTES: The following functions are not lock protected
// 

ssize_t ProcessManager::read(int fd, void* buf, size_t buflen, int timeout) {
    // poll readable
    pollfd pfd = {0};
    pfd.fd = fd;
    pfd.events = POLLIN;
    int ret = poll(&pfd, 1, timeout);

    // poll timeout
    if (ret == 0) {
        LOGD(TAG, "poll fd %d timeout", fd);
        return 0;
    }

    // stdout readable
    if (ret > 0 && (pfd.revents & POLLIN)) {
        ssize_t rv = ::read(fd, buf, buflen);
        if (rv == -1) {
            LOGD(TAG, "read fd %d failed: %s", fd, strerror(errno));
        } else if (rv == 0) {
            //LOGD(TAG, "read fd %d status: end of file", fd);
        }
        return rv;
    } 

    // poll error
    LOGD(TAG, "poll fd %d failed: %s", fd, strerror(errno));
    return -1;
}


ssize_t ProcessManager::write(int fd, const void* buf, size_t buflen, int timeout) {
    // poll writable
    pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLOUT;
    int ret = poll(&pfd, 1, timeout);

    // poll timeout
    if (ret == 0) {
        return 0;
    } 

    // stdin writeable
    if (ret > 0 && (pfd.revents & POLLOUT)) {
        ssize_t rv = ::write(fd, buf, buflen);
        if (rv == -1) {
            LOGD(TAG, "write fd %d failed: %s", fd, strerror(errno));
        } else if (rv == 0) {
            //LOGD(TAG, "write fd %d status: nothing was written", fd);
        }
        return rv;
    }

    // poll error
    LOGD(TAG, "poll fd %d failed: %s", fd, strerror(errno));
    return -1;
}




bool ProcessManager::probeAlive(pid_t pid) {
    int status = -1;
    int ret = waitpid(pid, &status, WNOHANG);
    if (ret == 0) {
        return true;
    }
    return false;
}



bool ProcessManager::recycle(pid_t pid) {
    auto it = processes.find(pid);
    if (it == processes.end()) {
        return false;
    }
    
    auto process = it->second;
    if (close(process->inFD()) == -1) {
        LOGE(TAG, "close process %d stdin error: %s", pid, strerror(errno));
    }
    if (close(process->outFD()) == -1) {
        LOGE(TAG, "close process %d stdout error: %s", pid, strerror(errno));
    }
    if (close(process->errFD()) == -1) {
        LOGE(TAG, "close process %d stderr error: %s", pid, strerror(errno));
    }
    
    processes.erase(process->id());
    LOGD(TAG, "process %d is recycled.", pid);
    return true;
}


pid_t ProcessManager::owner(int fd) {
    for (auto it = processes.begin(); it != processes.end(); ++it) {
        auto process = it->second;
        if (fd == process->inFD() || fd == process->outFD() || fd == process->errFD()) {
            return process->id();
        }
    }
    return -1;
}


bool ProcessManager::terminateAll() {
    bool status = true;
    for (auto process: processes) {
        // send SIGKILL to the child
        pid_t pid = process.first;
        int err = ::kill(pid, SIGTERM);
        if (err) {
            LOGE(TAG, "kill process %d with SIGTERM failed: %s", pid, strerror(errno));
            status = false;
        }
    }
    return status;
}


bool ProcessManager::killAll() {
    bool status = true;
    for (auto process: processes) {
        // send SIGTERM to the child
        pid_t pid = process.first;
        int err = ::kill(pid, SIGKILL);
        if (err) {
            LOGE(TAG, "kill process %d with SIGKILL failed: %s", pid, strerror(errno));
            status = false;
        }
    }
    return status;
}



