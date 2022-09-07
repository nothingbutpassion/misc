#pragma once



#define RECV_BUFFER_SIZE	256
#define SEND_BUFFER_SIZE	256


int server_socket(short port, int backlog);
int close_socket(int sockfd) ;

int accept_client(int server_sock);
void handle_client(int client_sock);






