#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <memory>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "tcp_client.h"


#define LOGD(...)  printLog("DEBUG", "TCPClient", __VA_ARGS__)
#define LOGE(...)  printLog("ERROR", "TCPClient", __VA_ARGS__)


void printLog(const char* level, const char* tag, const char *fmt, ...) {
    // format log str
    va_list ap;
    char logstr[1024];
    va_start(ap, fmt);
    vsnprintf(logstr, sizeof(logstr), fmt, ap);
    va_end(ap);

    // format time str
    char timestr[32];
    timeval tv;
    gettimeofday(&tv, NULL);
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&tv.tv_sec));
   
    // write format str to stderr
    fprintf(stderr, "%s.%03d %d %d %s %s %s\n", timestr, int(tv.tv_usec/1000), getpid(), int(syscall(SYS_gettid)), level, tag, logstr);
}



struct Timer {
    Timer() { 
        gettimeofday(&start, nullptr);
    }
    int duration() {
        timeval end = {0};
        gettimeofday(&end, nullptr);
        return int(end.tv_sec - start.tv_sec)*1000 + (int(end.tv_usec) - int(start.tv_usec))/1000;
    }
private:
    timeval start = {0};
};

static int waitReadable(int fd, int timeout) {
    fd_set readfds;
    fd_set exceptfds;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    FD_ZERO(&exceptfds);
    FD_SET(fd, &exceptfds);
    
    timeval tv;
    tv.tv_sec = timeout/1000;
    tv.tv_usec = (timeout%1000)*1000;
    int ret = select(fd + 1, &readfds, nullptr, &exceptfds, &tv);
    if (ret == -1 || FD_ISSET(fd, &exceptfds)) {
        LOGE("select for reading failed: %s", strerror(errno));
        return -1;
    }
    if (ret == 0) {
        LOGD("select for reading timeout: %dms", timeout);
        return 0;
    }
    return 1;
}


static int waitWriteable(int fd, int timeout) {
    fd_set writefds;
    fd_set exceptfds;
    FD_ZERO(&writefds);
    FD_SET(fd, &writefds);
    FD_ZERO(&exceptfds);
    FD_SET(fd, &exceptfds);
    
    timeval tv;
    tv.tv_sec = timeout/1000;
    tv.tv_usec = (timeout%1000)*1000;
    int ret = select(fd + 1, nullptr, &writefds, &exceptfds, &tv);
    if (ret == -1 || FD_ISSET(fd, &exceptfds)) {
        LOGE("select for writting failed: %s", strerror(errno));
        return -1;
    }
    if (ret == 0) {
        LOGD("select for writting timeout: %dms", timeout);
        return 0;
    }
    return 1;
}



TCPClient::TCPClient() {
}

TCPClient::~TCPClient() {
    release();
}


bool TCPClient::init(const char* ip, unsigned short port) {
    socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd == -1) {
        LOGE("socket failed: %s", strerror(errno));
        return false;
    }
    sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);
    if (connect(socketFd, (sockaddr*)&addr, sizeof(addr)) == -1) {
    	LOGE("connect failed: %s", strerror(errno));
        release();
    	return false;
    }
    LOGD("init ok: socket=%d, server=%s, port=%u", 
        socketFd, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    return true;

}


void TCPClient::release() {
    if (socketFd != -1) {
        close(socketFd);
        LOGD("release: socket=%d", socketFd);
        socketFd = -1;
    }   
}


int TCPClient::send(const void* buffer, unsigned int length, int timeout) {
    Timer timer; 
    int writed = 0;
    const char* buf = (const char*)buffer;
    while (writed < length) {
        // timeout
        int remaining = timeout - timer.duration();
        if (remaining <= 0) {
            LOGE("send timeout: %dms", timeout-remaining);
            return writed;
        }
            
        // select error
        int ret = waitWriteable(socketFd, remaining);
        if (ret == -1) {
            LOGE("send error", writed, length);
            return -1;
        }
        // select timeout
        if (ret == 0) {
            LOGE("send timeout: %dms", timeout-remaining);
            return writed;
        }
        // MSG_NOSIGNAL：
        // Don't  generate a SIGPIPE signal if the peer on a stream-oriented socket has closed the connection.  
        // The EPIPE error is still returned
        ret = ::send(socketFd, buf + writed, length-writed, MSG_NOSIGNAL|MSG_DONTWAIT);
        if (ret == -1) {
            // interrupted or try again
            if (errno == EAGAIN || errno == EINTR)
                continue;
            LOGE("send failed: %s", strerror(errno));
            return -1;
        } else if (ret == 0) {
            // signal writeable, but return 0
            LOGE("send return 0");
            continue;
        }
        writed += ret;
        LOGD("send %d/%d", writed, length);
    }
    return writed;
}

int TCPClient::recv(void* buffer, unsigned int length, int timeout) {
    Timer timer;
    int read = 0;
    char* buf = (char*)buffer;
    while (read < length) {
        // timeout
        int remaining = timeout - timer.duration();
        if (remaining <= 0) {
            LOGE("recv timeout: %dms", timeout-remaining);
            return read;
        }
        // select error
        int ret = waitReadable(socketFd, remaining);
        if (ret == -1) {
            LOGE("recv error");
            return -1;
        }
        // select timeout
        if (ret == 0) {
            LOGE("recv timeout: %dms", timeout-remaining);
            return read;
        }
        ret = ::recv(socketFd, buf + read, length-read, MSG_DONTWAIT);
        if (ret == -1) {
            // interrupted or try again
            if (errno == EAGAIN || errno == EINTR)
                continue;
            LOGE("recv failed: %s", strerror(errno));
            return -1;
        } else if (ret == 0) {
            // signal readable, but return 0, the peer socket closed.
            LOGE("recv 0 bytes: socket %d closed by peer", socketFd);
            return -1;
        }
        read += ret;
        LOGD("recv %d/%d", read, length);
    }
    return read;
}



int main(int argc, char* argv[]) {
    if (argc != 4) {
        LOGE("usage: %s, <ip> <port> <msg>", argv[0]);
        return -1;
    }

    std::shared_ptr<TCPClient> tcp(new TCPClient, [](TCPClient* p) { p->release(); delete p; });
    if (!tcp->init(argv[1], atoi(argv[2])))
        return -1;

    std::string msg = argv[3];
    if (tcp->send(msg.c_str(), msg.size()) != msg.size())
        return -1;
    LOGD("sent: %s", msg.c_str());
    
    char buffer[1024] = {0};
    tcp->recv(buffer, sizeof(buffer), 10000);
    LOGD("received: %s", buffer);
    return 0;
}



