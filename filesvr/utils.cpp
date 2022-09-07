#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>

#include <string>

#include "log.h"
#include "utils.h"


using namespace std;



#define RECV_BUFFER_SIZE	256
#define SEND_BUFFER_SIZE	256
#define MAX_REQUEST_SIZE    0x400000
#define TAG "Utils"


int server_socket(short port, int backlog) {
    // Create a tcp socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        LOGE(TAG, "socket failed: %s", strerror(errno));
        return -1;
    }

    // Bind socket with specified port
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind(sockfd , (sockaddr*)&addr, sizeof(addr)) == -1) {
        
        LOGE(TAG, "bind failed: %s", strerror(errno));
        close(sockfd);
        return -1;
    }
    
    // Listen client connections
    if (listen(sockfd , backlog) == -1) {
        LOGE(TAG, "liste failed: %s", strerror(errno));
        close(sockfd);
        return -1;
    }

    return sockfd;
}


int close_socket(int sockfd) {
    return close(sockfd);
}


int accept_client(int server_sock) {
    sockaddr_in client_addr;
    socklen_t client_addr_len;
    int client_sock = accept(server_sock, (sockaddr*)&client_addr, &client_addr_len);
    if (client_sock == -1) {
    	LOGE(TAG, "accept failed: %s", strerror(errno));
    }
    
    LOGI(TAG, "accepted socket %d from %s:%u", client_sock, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    return client_sock;
}


/**
 * File upload and download protocol
 */

struct upload_request {
    string cmd;
    string path;
    string size;
    string data;
};

struct download_request {
    string cmd;
    string path;
};

/*
struct download_response {
    string size;
    string data;
};
*/


string translate_path(const string& path) {
    size_t pos = path.rfind("/");
    if (pos != string::npos) {
        return path.substr(pos+1);
    }

    pos = path.rfind("\\");
    if (pos != string::npos) {
        return path.substr(pos+1);
    }
    return path;
}


void save_file(const string& path, const string& data) {
    FILE* fp = fopen(path.c_str(), "wb");
    if (!fp) {
        LOGE(TAG, "can't open %", path.c_str());
        return;
    }

    size_t writed = fwrite(data.data(), 1, data.size(), fp);
    if (writed != data.size()) {
        LOGE(TAG, "fwrite %u bytes to %s failed", writed, path.c_str());
    }
    fclose(fp);

    LOGI(TAG, "%s is saved", path.c_str());
}



void send_file(const string& path, int sockfd) {
    FILE* fp = fopen(path.c_str(), "rb");
    if (!fp) {
        LOGE(TAG, "can't open %s", path.c_str());

        // act as file size is 0
        ssize_t ns = send(sockfd, "0\r\n", 3, 0);
        LOGE(TAG, "send %u bytes to socket %d", ns, sockfd);
        return;
    }

    // get file size
    fseek(fp, 0L, SEEK_END);
    size_t filelen = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    // send file size
    char send_buf[SEND_BUFFER_SIZE];
    sprintf(send_buf, "%u\r\n", filelen);
    ssize_t ns = send(sockfd, send_buf, strlen(send_buf), 0);
    if (ns == -1) {
        LOGE(TAG, "send size to socket %d failed: %s", sockfd, strerror(errno));
        fclose(fp);
        return;
    } 

    // send file data
    size_t read = 0;
    while (read < filelen) {
        size_t nr = fread(send_buf, 1, sizeof(send_buf), fp);
        if (nr == 0) {
            LOGE(TAG, "read 0 bytes from %s: %s", path.c_str() , strerror(errno));
            break;
        } 

        ssize_t ns = send(sockfd, send_buf, nr, 0);
        if (ns != nr) {
            LOGE(TAG, "send %u bytes data to socket %d failed: %s", nr, sockfd, strerror(errno));
            break;
        }
        read += nr;
    }
    
    fclose(fp);

    if (read < filelen) {
        LOGE(TAG, "%s (only %u bytes) is sent to socket %d ", path.c_str(), read, sockfd);
    } else {
        LOGI(TAG, "%s (total %u bytes) is sent to socket %d ", path.c_str(), read, sockfd);
    }
}




void handle_upload(const upload_request& upload, int sockfd) {
    string path = translate_path(upload.path);
    save_file(path, upload.data);
}


void handle_download(const download_request& download, int sockfd) {
    string path = translate_path(download.path);
    send_file(path, sockfd);
}


void handle_request(const string& request, int sockfd) {
    if (0 == request.find("upload\r\n")) {
        // cmd
        upload_request upload;
        upload.cmd = "upload";

        // path
        size_t start = upload.cmd.size() + 2;
        size_t pos = request.find("\r\n", start);
        if (pos == string::npos) {
            LOGE(TAG, "can't resolve upload path");
            return;
        }
        upload.path = request.substr(start, pos - start);
        start = pos + 2; 

        // size
        pos = request.find("\r\n", start);
        if (pos == string::npos) {
            LOGE(TAG, "can't resolve upload size");
            return;
        }
        upload.size = request.substr(start, pos - start);
        start = pos + 2;

        // data 
        upload.data = request.substr(start);
        
        LOGI(TAG, "cmd=%s, path=%s, size=%s, data.size=%d", 
            upload.cmd.c_str(), upload.path.c_str(), upload.size.c_str(), upload.data.size());

        handle_upload(upload, sockfd);
    } else if (0 == request.find("download\r\n")) {
        // cmd
        download_request download;
        download.cmd = "download";

        // path
        size_t start = download.cmd.size() + 2;
        size_t pos = request.find("\r\n", start);
        if (pos == string::npos) {
            LOGE(TAG, "can't resolve download path");
            return;
        }
        download.path = request.substr(start, pos - start);
        LOGI(TAG, "cmd=%s, path=%s", download.cmd.c_str(), download.path.c_str());

        handle_download(download, sockfd); 
    } else {
        LOGE(TAG, "error request");
    }
}


void* handle_thread(void* user) {
    int client_sock = int(uintptr_t(user));
    string recv_str;
    char recv_buf[RECV_BUFFER_SIZE];
    while (1)  {
        ssize_t nr = recv(client_sock, recv_buf, sizeof(recv_buf), 0);
        if (nr == -1) {
            LOGE(TAG, "recv failed: %s", strerror(errno));
            break;
        }
        if (nr == 0) {
            LOGD(TAG, "recv completed: end-of-file");
            break;
        }
        recv_str += string(recv_buf, recv_buf + nr);

        // Avoid memory overflow
        if (recv_str.size() > MAX_REQUEST_SIZE) {
            LOGE(TAG, "received data size has grater than 4M");
            break;
        }
    }

    LOGD(TAG, "received %d bytes from socket %d [\"%s\"]", recv_str.size(), client_sock, recv_str.c_str());
    handle_request(recv_str, client_sock);
    close(client_sock);
    return NULL;
}


void handle_client(int client_sock) {
    pthread_attr_t tattr;
    pthread_t tid;
    pthread_attr_init(&tattr);
    pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&tid, &tattr, handle_thread, (void*)client_sock) != 0) {
        LOGE(TAG, "pthread_create failed: %s", strerror(errno));
        close(client_sock);
    }
}

