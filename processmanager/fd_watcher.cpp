#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/epoll.h>
#include "fd_watcher.h"
#include "framework_log.h"
#include "framework_utils.h"
#include "process_manager.h"


// tag for this file
#define TAG "FDWatcher"

// epoll fd timeout
#define EPOLL_TIMEOUT 300


namespace {

// epoll fd used to poll I/O events.
int epoll_fd = -1;

// wather thread id
pthread_t watcher_tid = -1; 

// ask the thread stop
volatile bool need_stop = false;
// Mutex need_stop_mutex;

void poll_error(int fd) {
    // delete the error fd from poll
    int err = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    if (err) {
        // notify process manager fd poll error.
        LOGE(TAG, "epoll_ctl delete fd %d error: %s", fd, strerror(errno));
        ProcessManager::Event event;
        event.type = ProcessManager::FD_POLL_ERR;
        event.data.fd = fd;
        ProcessManager::instance().notify(event);
    } 
}

void* watcher_thread(void* data) {
    LOGI(TAG, "watcher thread is entered.");
    while (true) {
        // whether need stop
        {//Lock lock(need_stop_mutex);
            if (need_stop) {
                LOGI(TAG, "wather thread is exited by command.");
                return 0;
            }
        }

        // wait process exited notification
        epoll_event ev = {0};
        int ret = epoll_wait(epoll_fd, &ev, 1, EPOLL_TIMEOUT);
        if (ret < 0) {
            LOGE(TAG, "wather thread epoll_wait failed: %s", strerror(errno));
        } else if (ret == 0) {
            //LOGD(TAG, "epoll_wait timeout.");
        } else {
            if ((ev.events & EPOLLERR) || (ev.events & EPOLLHUP)) {
                poll_error(ev.data.fd);
            } else if (ev.events & EPOLLIN) {
                // LOGI(TAG, "fd %d readable", ev.data.fd);
                char buf[256];
                int len = read(ev.data.fd, buf, sizeof(buf));
                if (len > 0) {
                    if (write(STDERR_FILENO, buf, len) == -1) {
                       // LOGW(TAG, "write parent's stderr failed: %s", strerror(errno) ); 
                    }
                }
                if (len == -1 && errno != EAGAIN) {
                    poll_error(ev.data.fd);
                } 
            } else if (ev.events & EPOLLOUT) {
                //LOGI(TAG, "fd %d writeable", ev.data.fd);
            }
        }
    }
    LOGI(TAG, "watcher thread is exited.");
    return 0;
}


}; // anonymous namespace


FDWatcher::FDWatcher() {
    if (!init()) {
        exit(EXIT_FAILURE);
    }
}

FDWatcher::~FDWatcher() {
    release();
}


bool FDWatcher::start() {
    {//Lock lock(need_stop_mutex);
        need_stop = false;
    }
    int err = pthread_create(&watcher_tid, NULL, watcher_thread, this);
    if (err) {
        LOGE(TAG, "pthread_create failed: %s", strerror(err));
        return false;
    }
    return true;
}

bool FDWatcher::stop() {
    {//Lock lock(need_stop_mutex);
        need_stop = true;
    }
    int err = pthread_join(watcher_tid, NULL);
    if (err) {
        LOGE(TAG, "pthread_join failed: %s", strerror(err));
        return false;
    }
    return true;
}


bool FDWatcher::add(int type, int fd) {
    // check input args
    if (type < FD_READ || type > FD_WRITE) {
        LOGE(TAG, "invalid fd type: %d", type);
        return false;
    }

    // add fd to epoll
    epoll_event ev = {0};
    ev.events = (type == FD_READ) ? EPOLLIN : (EPOLLERR | EPOLLHUP);
    ev.data.fd = fd;
    int err = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
    if (err) {
        LOGE(TAG, "epoll_ctl add fd failed: %s\n", strerror(errno));
        return false;
    }
    return true;
}

bool FDWatcher::init() {
    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        LOGE(TAG, "epoll_create1 failed: %s", strerror(errno));
        return false;
    }
    return true;
}

bool FDWatcher::release() {
    int err = close(epoll_fd);
    if (err) {
        LOGE(TAG, "close epoll fd failed: %s", strerror(errno));
        return false;
    }
    return true;
}


