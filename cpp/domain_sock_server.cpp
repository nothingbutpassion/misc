#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#define BUF_SIZE 256

/* Maximum size of messages exchanged between client to server */
#define SV_SOCK_PATH "/tmp/ud_ucase"

int main(int argc, char *argv[])
{
    struct sockaddr_un svaddr;      // server socket
    struct sockaddr_un claddr;      // client socket
    char buf[BUF_SIZE];

    /* Create UPD socket */
    int sfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sfd == -1) {
        printf("socket failed\n");
        exit(-1);
    }

    /* Create server socket */
    /* Construct well-known address and bind server socket to it */
    if (remove(SV_SOCK_PATH) == -1 && errno != ENOENT) {
        printf("remove failed\n");
        exit(-1);
    }
    memset(&svaddr, 0, sizeof(struct sockaddr_un));
    svaddr.sun_family = AF_UNIX;
    strncpy(svaddr.sun_path, SV_SOCK_PATH, sizeof(svaddr.sun_path) - 1);
    if (bind(sfd, (struct sockaddr *) &svaddr, sizeof(struct sockaddr_un)) == -1) {
        printf("bind failed\n");
        exit(-1);
    }

    /* Receive messages, convert to uppercase, and return to client */
    for (;;) {
        socklen_t len = sizeof(struct sockaddr_un);
        ssize_t numBytes = recvfrom(sfd, buf, BUF_SIZE, 0, (struct sockaddr *) &claddr, &len);
        if (numBytes == -1) {
            printf("recvfrom failed\n");
            exit(-1);
        }
        buf[numBytes] = '\0';
        printf("Server received %ld bytes(%s) from %s\n", (long) numBytes,  buf, claddr.sun_path);

        // Convert received to upper
        for (int j = 0; j < numBytes; j++) {
            buf[j] = toupper((unsigned char) buf[j]);
        }
        if (sendto(sfd, buf, numBytes, 0, (struct sockaddr *) &claddr, len) != numBytes) {
            printf("sendto failed\n");
            exit(-1);
        }
    }
}
