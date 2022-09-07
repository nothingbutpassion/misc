#pragma once

struct TCPServer {
    TCPServer();
    ~TCPServer();
    bool init(unsigned short port, int backlog=1);
    void release();
    int accept(int timeout=3000);
    int send(int sockfd, const void* buffer, unsigned int length, int timeout=3000);
    int recv(int sockfd, void* buffer, unsigned int length, int timeout=3000);
private:
    int socketFd = -1;

};


