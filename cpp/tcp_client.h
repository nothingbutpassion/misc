#pragma once


#pragma once

struct TCPClient {
    TCPClient();
    ~TCPClient();
    bool init(const char* server, unsigned short port);
    void release();
    int send(const void* buffer, unsigned int length, int timeout=3000);
    int recv(void* buffer, unsigned int length, int timeout=3000);
private:
    int socketFd = -1;

};