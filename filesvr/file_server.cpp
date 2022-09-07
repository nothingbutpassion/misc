#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "utils.h"
#include "log.h"

#define SERVER_PORT         6006
#define CLIENT_BACKLOG      16
#define TAG                 "FileServer"


int main(int argc, char* argv[]) {

    // Create server socket
    int server_sock = server_socket(SERVER_PORT, CLIENT_BACKLOG);
    if (server_sock == -1) {
        LOGE(TAG, "create server socket failed");
        exit(-1);
    }

    // Ignor SIGPIPE which may be caused by send or recv
    signal(SIGPIPE, SIG_IGN);

    // Accept and handle client requests 
    for ( ; ; ) { 
        
        // Accept one client
        int client_sock = accept_client(server_sock);
        if (client_sock == -1) {
            LOGE(TAG, "accept client socket failed: %s", strerror(errno));
            close_socket(server_sock);
            exit(-1);
        }

        // Handle the client
        handle_client(client_sock);
        
    }

    // Close server socket
    close_socket(server_sock);
    return 0;
}
