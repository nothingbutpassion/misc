#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <string>
#include "framework_log.h"
#include "framework_utils.h"
#include "signal_watcher.h"
#include "process_manager.h"



// tag for this file
#define TAG "SignalWatcher"

// epoll signal fd timeout
#define EPOLL_TIMEOUT 300


namespace {

// socketpair read/write fds
int signal_read_fd = -1;
int signal_write_fd = -1;

// epoll fd
int epoll_fd = -1;

// watcher pthread id
pthread_t watcher_tid = -1;

// ask the pthread stop
volatile bool need_stop = false;
//Mutex need_stop_mutex;


std::string describe_status(int status) {
    char buf[64];
    if (WIFEXITED(status)) {
        sprintf(buf, "exited with status %d", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        sprintf(buf, "killed by signal %d %s", WTERMSIG(status), WCOREDUMP(status) ? "(core dumped)": "");
    } else if (WIFSTOPPED(status)) {
        sprintf(buf, "stopped by signal %d", WSTOPSIG(status));
    } else if (WIFCONTINUED(status)) {
        sprintf(buf, "continued");
    } else {
        sprintf(buf, "unknown status");
    }
    return std::string(buf);
}

void signal_handler(int signo) {
    LOGD(TAG, "signal handler received signal: %d (%s)", signo, strsignal(signo));

    // do nothing but writting a byte to notify wather thread
    if (write(signal_write_fd, "1", 1) == -1) {
        LOGD(TAG, "signal handler write failed: %s", strerror(errno));
    }
}


void* watcher_thread(void* arg) {
    LOGI(TAG, "watcher thread is entered");

    // The watcher thread is responsible for SIGCHLD
    // Some guy surggest using sigwait() in this thread
    sigset_t blockSignals;
    sigemptyset(&blockSignals);	
    sigaddset(&blockSignals, SIGCHLD);	
    pthread_sigmask(SIG_UNBLOCK, &blockSignals, NULL);

    while (true) {
        {//Lock lock(need_stop_mutex);
            if (need_stop) {
                LOGI(TAG, "wather thread is exited by command");
                return 0;
            }
        }

        // wait event
        epoll_event event;
        int ret = epoll_wait(epoll_fd, &event, 1, EPOLL_TIMEOUT);

        // epoll wait timeout
        if (ret == 0) {
            //LOGD(TAG, "wather thread epoll_wait timeout");
            continue;
        }

        // epoll wait error
        if (ret == -1 && errno != EINTR) {
            LOGD(TAG, "wather thread epoll_wait failed: %s", strerror(errno));
            continue;
        }

        //
        // here means: 1) an event arrived. 2) epoll_wait interrupted by signal
        //

        // do nothing but just reading
        char buf[32];
        if (read(signal_read_fd, buf, sizeof(buf)) == -1) {
            LOGD(TAG, "wather thread read failed: %s", strerror(errno));
        }

        // try to wait all child process
        int pid = -1;
        int status = -1;
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            LOGI(TAG, "wather thread waited pid: %d %s", pid, describe_status(status).c_str());
            
            // notify process manager to recycle 
            ProcessManager::Event event;
            event.type = ProcessManager::SIG_CHLD_EXIT;
            event.data.pid = pid;
            ProcessManager::instance().notify(event);
        }
    }
    LOGI(TAG, "watcher thread is exited");
    return 0;
}



}; // anonymous namespace 



SignalWatcher::SignalWatcher() {
    if (!init()) {
        exit(EXIT_FAILURE);
    }
}


SignalWatcher::~SignalWatcher()  {
    release();
}


bool SignalWatcher::start() {
    {//Lock lock(need_stop_mutex);
        need_stop = false;
    }
    int err = pthread_create(&watcher_tid, NULL, watcher_thread, NULL);
    if (err) {
        LOGE(TAG, "pthread_create failed: %d", strerror(err));
        return false;
    }
    return true;
}

bool SignalWatcher::stop() {
    {//Lock lock(need_stop_mutex);
        need_stop = true;
    }
    int err = pthread_join(watcher_tid, NULL);
    if (err) {
        LOGE(TAG, "pthread_join failed: %d", strerror(err));
        return false;
    }
    return true;
}

bool SignalWatcher::init() {
    // we ignor the following signals
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        LOGE(TAG, "signal(SIGPIPE, SIG_IGN) failed: %s", strerror(errno));
        return false;
    }
    if (signal(SIGTTIN, SIG_IGN) == SIG_ERR) {
        LOGE(TAG, "signal(SIGTTIN, SIG_IGN) failed: %s", strerror(errno));
        return false;
    }
    if (signal(SIGTTOU, SIG_IGN) == SIG_ERR) {
        LOGE(TAG, "signal(SIGTTOU, SIG_IGN) failed: %s", strerror(errno));
        return false;
    }
    
    // create socket pair for communication with registed signal handler
    int s[2];
    int err = socketpair(AF_UNIX, SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC, 0, s);
    if (err) {
        LOGE(TAG, "socketpair failed: %s", strerror(errno));
        return false;
    }
    signal_write_fd = s[0];
    signal_read_fd = s[1];

    // set signal mask to mask out SIGCHLD in this thread and all the threads 
    // that create by this thread	
    sigset_t blockSignals;
    sigemptyset(&blockSignals);	
    sigaddset(&blockSignals, SIGCHLD);	
    pthread_sigmask(SIG_BLOCK, &blockSignals, NULL);

    // install signal handler for SIGCHLD
    struct sigaction act = {0};
    act.sa_handler = signal_handler;
    act.sa_flags = SA_NOCLDSTOP;
    err = sigaction(SIGCHLD, &act, NULL);
    if (err) {
        LOGE(TAG, "sigaction failed: %s", strerror(errno));
        close(signal_read_fd);
        close(signal_write_fd);
        return false;
        
    } 
    
    // create epoll 
    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        LOGE(TAG, "epoll_create1 failed: %s", strerror(errno));
        close(signal_read_fd);
        close(signal_write_fd);
        return false;
    }
    
    // add sigal read fd to epoll
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = signal_read_fd;
    err = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, signal_read_fd, &ev);
    if (err) {
        LOGE(TAG, "epoll_ctl failed: %s", strerror(errno));
        close(signal_read_fd);
        close(signal_write_fd);
        close(epoll_fd);
        return false;
    }

    return true;
}


bool SignalWatcher::release() {
    
    // delete signal read fd from epoll
    int err = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, signal_read_fd, NULL);
    if (err) {
        LOGE(TAG, "epoll_ctl failed: %s", strerror(errno));
    }

    // close the communication sokect
    err |= close(signal_write_fd);
    err |= close(signal_read_fd);
    err |= close(epoll_fd);
    return (err == 0);
}



