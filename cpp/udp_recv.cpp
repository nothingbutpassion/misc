#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>

void showUsage(const char* appName) {
    printf("usage: %s <port>\n", appName);
}


int main(int argc, char** argv) {
    if (argc != 2) {
        showUsage(argv[0]);
        return -1;
    }

    unsigned short port  = (unsigned short)atoi(argv[1]);
    int socketFD = socket(AF_INET, SOCK_DGRAM, 0);
    if (!socketFD) {
        perror("create socket failed");
        return -1;
    }

    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    if (bind(socketFD, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    	perror("bind failed");
    	return -1;
    }

    for (;;) {
        printf("waiting on port %d\n", port);
        char buf[1024];
        struct sockaddr_in addr;
        socklen_t addrlen;
        int recvlen = recvfrom(socketFD, buf, sizeof(buf), 0, (struct sockaddr *)&addr, &addrlen);
        if (recvlen > 0) {
            buf[recvlen] = 0;
            unsigned short portNumber = ntohs(addr.sin_port);
            const char* ipAddr = inet_ntoa(addr.sin_addr);
            printf("received %d bytes(\"%s\") from %s:%u\n", recvlen, buf, ipAddr, portNumber);
        }
        if (recvlen < 0) {
            perror("recvfrom error");
            break;
        }
    }

    close(socketFD);

    return 0;
}

