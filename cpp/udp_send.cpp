#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>


void showUsage(const char* appName) {
    printf("usage: %s <ip> <port> <contents>\n", appName);
}

int main(int argc, char** argv) {
    if (argc != 4) {
        showUsage(argv[0]);
        return -1;
    }

    int socketFD = socket(AF_INET, SOCK_DGRAM, 0);
    if (!socketFD) {
        perror("can't create socket");
        return -1;
    }

    const char* ip = argv[1];
    unsigned short port = (unsigned short)atoi(argv[2]);
    const char* contents = argv[3];

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);
    int sent = sendto(socketFD, contents, strlen(contents), 0, (const struct sockaddr*)&addr, sizeof(addr));
    if (sent < 0) {
    	perror("sendto failed");
    	return -1;
    }
    printf("send %d bytes(\"%s\") to %s:%u\n", sent, contents, ip, port);

    close(socketFD);
    return 0;
}
