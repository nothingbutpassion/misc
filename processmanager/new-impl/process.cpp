#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <string>

#include "process.h"
#include "log.h"


using namespace std;

#define TAG "Process"

IOEType Process::toType(const string& name) {
    if (name == "") {
        return IOE_NONE;
    }
    if (name == "|") {
        return IOE_PIPE;
    }
    return IOE_FILE;
}

bool Process::create(const string& file, const vector<string>& args,
    const string& inName, const string& outName, const string& errName) {

    this->file = file;
    this->args = args;
    ioes[0].name = inName;
    ioes[1].name = outName;
    ioes[2].name = errName;

    // Create fds for stdin/out/err name
    if (!init()) {
        release();
        return false;
    }
    
    // Create pipe for communication if exec failed
    int pexec[2] = {-1, -1};
    if (pipe(pexec) == -1) {
        LOGE(TAG, "create pipe failed: %s", strerror(errno));
        release();
        return false;
    }
    if (fcntl(pexec[READ], F_SETFD, FD_CLOEXEC) == -1 
        || fcntl(pexec[WRITE], F_SETFD, FD_CLOEXEC) == -1) {
        LOGE(TAG, "fcntl pipe failed: %s", strerror(errno));
        close(pexec[READ]);    
        close(pexec[WRITE]);
        release();
        return false;
    }


    // fork
    pid = fork();
    if (pid == -1) {
        close(pexec[READ]);    
        close(pexec[WRITE]);
        release();
        return false;
    }

    // Child process
    if (pid == 0) {
        // close all fds execept 0, 1, 2, pexec[WRITE]
        int maxfd = sysconf(_SC_OPEN_MAX);
        for(int fd = 3; fd < maxfd; fd++) {
            if (fd != pexec[WRITE])
            close(fd);
        }

        // exec system call
        if (-1 == exec()) {
            //LOGE(TAG, "exec failed: %s", strerror(errno));
            int error = errno;
            while (write(pexec[WRITE], &error, sizeof(error)) == -1 
                && errno == EINTR){ 
            }
            close(pexec[WRITE]);
            exit(error);
        } 
    }

    // Parent process
    close(pexec[WRITE]);
    int error;
    int ret = read(pexec[READ], &error, sizeof(error));
    if (ret > 0 || ret == -1) {
        LOGE(TAG, "exec failed: %s", ret > 0 ? strerror(error) : strerror(errno));
        close(pexec[READ]);
        release();
        return false;
    }

    // succeed
    close(pexec[READ]);    
    return true;
}

int Process::fork() {
    // fork a child process
    pid_t pid = ::fork();
    if (pid == -1) {
        LOGE(TAG, "fork failed: %s", strerror(errno));
        return pid;
    }

    // Child process
    if (pid == 0) {
        // For stdin/stdout/stderr
        for (int i=0; i <= 2; i++) {
            IOE& ioe = ioes[i];
            if (ioe.type == IOE_FILE || ioe.type == IOE_PIPE) {
                if (i == 0) {
                    dup2(ioe.fds[READ], i);
                } else {
                    dup2(ioe.fds[WRITE], i);
                }
                for (int fd : ioe.fds) {
                    close(fd);
                }
            }
        }
        return pid;
    }

    // Parent process
    // For stdin/stdout/stderr
    for (int i=0; i <= 2; i++) {
        IOE& ioe = ioes[i];
        if (ioe.type == IOE_FILE || ioe.type == IOE_PIPE) {
            if (i == 0) {
                close(ioe.fds[READ]);
                ioe.fds[READ] = -1;
            } else {
                close(ioe.fds[WRITE]);
                ioe.fds[WRITE] = -1;
            }
        }
    }
    return pid;
}


int Process::exec() {
    char** argv = new char*[args.size()+1];
    for (size_t i = 0; i < args.size(); ++i) {
        const string& src = args[i];            
        char*& dest = argv[i];
        dest = new char[src.size()+1];
        dest[src.copy(dest, src.size())] = '\0';
    }
    argv[args.size()] = NULL;
    return execvp(file.c_str(), argv);
}



bool Process::init() {
    
    // For stdin/stdout/stderr
    for (int i=0; i <=2; i++) {
        IOE& ioe = ioes[i];
        ioe.type = toType(ioe.name);
        if (ioe.type == IOE_FILE) {     
            if (i == 0) {
                // For stdin
                ioe.fds[READ] = open(ioe.name.c_str(), O_RDONLY|O_TRUNC|O_CREAT, S_IRUSR|S_IWUSR);
                if (ioe.fds[READ]== -1) {
                    LOGE(TAG, "open %s failed: %s", ioe.name.c_str(), strerror(errno));
                    return false;
                }
                ioe.fds[WRITE] = open(ioe.name.c_str(), O_WRONLY|O_APPEND);
            } else {
                // For stdout/stderr
                ioe.fds[WRITE] = open(ioe.name.c_str(), O_WRONLY|O_TRUNC|O_CREAT, S_IRUSR|S_IWUSR);
                if (ioe.fds[WRITE]== -1) {
                    LOGE(TAG, "open %s failed: %s", ioe.name.c_str(), strerror(errno));
                    return false;
                }
                ioe.fds[READ] = open(ioe.name.c_str(), O_RDONLY);

            }
        } else if (ioe.type == IOE_PIPE) {
            if (pipe(ioe.fds) == -1) {
                LOGE(TAG, "create pipe failed: %s", strerror(errno));
                return false;
            }
        }
    }
    return true;
}

void Process::release() {
    for (IOE& ioe: ioes) {
        close(ioe.fds[READ]);
        close(ioe.fds[WRITE]);
        ioe.fds[READ] = -1;
        ioe.fds[WRITE] = -1;
        ioe.name = "";
        ioe.type = IOE_NONE;
    }
}


void Process::dump() {
    string as;
    for (const string& arg : args) {
        as += arg + " "; 
    }
    LOGD(TAG, "child: {file:%s, args:%s, pid:%d}", 
        file.c_str(), as.c_str(), pid);
    
    for (const IOE& ioe: ioes) {
        LOGD(TAG, "child: {name:%s, type:%d, fds:[%d,%d]}", 
            ioe.name.c_str(), ioe.type, ioe.fds[READ], ioe.fds[WRITE]);
    }
}

bool Process::isAlive() {
    if (pid == -1) {
        return false;
    }
    
    bool alive = waitpid(pid, NULL, WNOHANG) == 0;
    if (!alive) {
        //
        // Child is dead, close related fds.
        //
        release();
        pid = -1;
    }
    return alive;
}


bool Process::kill(int signal) {
    //
    // NOTES: 
    // The pid is not reused, because pid is direct child of this process.
    // Even if the child exited, the pid is reserved (this is what a "zombie process" is) until the parent waits on it
    //
    if (-1 == ::kill(pid, signal)) {
        LOGE(TAG, "kill failed: %s", strerror(errno));
        return false;
    }
    return true;
}


