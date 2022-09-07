#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#define BUF_SIZE 256

/* Maximum size of messages exchanged between client to server */
#define SV_SOCK_PATH "/tmp/ud_ucase"


int main(int argc, char *argv[]) {
    struct sockaddr_un svaddr;      // server socket address
    struct sockaddr_un claddr;      // client socket address
    char resp[BUF_SIZE];
    if (argc < 2 || strcmp(argv[1], "--help") == 0) {
        printf("%s msg...\n", argv[0]);
        exit(-1);
    }

    /* Create udp socket; bind to unique pathname (based on PID) */
    int sfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sfd == -1) {
        printf("socket failed\n");
        exit(-1);
    }
    memset(&claddr, 0, sizeof(struct sockaddr_un));
    claddr.sun_family = AF_UNIX;
    snprintf(claddr.sun_path, sizeof(claddr.sun_path),"/tmp/ud_ucase_cl.%ld", (long) getpid());
    if (bind(sfd, (struct sockaddr *) &claddr, sizeof(struct sockaddr_un)) == -1) {
        printf("socket failed\n");
        exit(-1);
    }

    /* Construct address of server */
    memset(&svaddr, 0, sizeof(struct sockaddr_un));
    svaddr.sun_family = AF_UNIX;
    strncpy(svaddr.sun_path, SV_SOCK_PATH, sizeof(svaddr.sun_path) - 1);

    /* Send messages to server; echo responses on stdout */
    for (int j = 1; j < argc; j++) {
        size_t msgLen = strlen(argv[j]);

        /* May be longer than BUF_SIZE */
        if (sendto(sfd, argv[j], msgLen, 0, (struct sockaddr *) &svaddr, sizeof(struct sockaddr_un)) != msgLen) {
            printf("sendto failed\n");
            exit(-1);
        }

        int numBytes = recvfrom(sfd, resp, BUF_SIZE, 0, NULL, NULL);
        if (numBytes == -1) {
            printf("recvfrom failed\n");
            exit(-1);          
        }
        resp[numBytes] = '\0';

        printf("Client received %d bytes(%s) from %s\n", (int) numBytes, resp, svaddr.sun_path);
    }

    /* Remove client socket pathname */
    remove(claddr.sun_path);
    return 0;   
}
