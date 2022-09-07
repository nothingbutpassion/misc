#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define show_info(...)          fprintf(stdout, __VA_ARGS__)
#define show_error(...)         fprintf(stderr, __VA_ARGS__)
#define show_error_exit(...)    do { show_error(__VA_ARGS__); exit(-1); } while (0)


#define SERVER_SOCKET_PORT      6006
#define RECV_BUFFER_SIZE        256
#define SEND_BUFFER_SIZE        256


int main(int argc, char* argv[]) {

    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        show_error_exit("create socket failed: %s\n", strerror(errno));
    }

    // Conect to server
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(SERVER_SOCKET_PORT);
    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        show_error_exit("connect to server failed: %s\n", strerror(errno));
    }

    // Send message
    char send_buf[SEND_BUFFER_SIZE] = "Hello server!";
    ssize_t ns = send(sock, send_buf, strlen(send_buf), 0);
    if (ns == -1) {
        show_error_exit("send failed: %s\n", strerror(errno));
    }
    show_info("send %d bytes(\"%s\") to %s:%u\n", ns, send_buf,  
        inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));


    // Receive message
    char recv_buf[RECV_BUFFER_SIZE];
    ssize_t nr = recv(sock, recv_buf, sizeof(recv_buf), 0);
    if (nr == -1) {
        show_error_exit("recv failed: %s\n", strerror(errno));
    }
    if (nr == 0) {
        show_error_exit("recv failed: end-of-file\n");
    }
    recv_buf[nr] = '\0';
    show_info("received %d bytes(\"%s\") from %s:%u\n", nr, recv_buf,  
        inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));

    
    // Close socket
    if (close(sock) == -1) {
        show_error_exit("close socket failed: %s\n", strerror(errno));    
    }
    
  
    return 0;
}

