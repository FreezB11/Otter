///@file: proxy.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define BUFFER 4096*4
#define MAX_HEADERS 50

typedef struct{
    char method[16];
    char url[2048];
    char version[16];
    char host[256];
    int port;
    char path[2048];
    char headers[MAX_HEADERS][2][256];
    int header_count;
}HttpRequest;

ssize_t read_http_headers(int fd, char* buf, int maxlen){
    ssize_t total = 0;
    while(total < maxlen - 1){
        ssize_t n = recv(fd, buf + total, 1, 0);
        if(n <= 0) return n;
        total += n;
        buf[total] = '\0';
        
        if(total >= 0 && strcmp(buf + total - 4, "\r\n\r\n") == 0)
            break;
    }
    return total;
}

int parseREQ(const char *raw, HttpRequest *req){
    memset(req, 0, sizeof(HttpRequest));
    req->port = 80; // default;
    char raw_copy[BUFFER];
    strncpy(raw_copy, raw, BUFFER - 1);

    char *line_end = strstr(raw_copy, "\r\n");
    if(!line_end) return -1;
    *line_end = '\0';

    if(sscanf(raw_copy, "%15s %2047s %15s", req->method, req->url, req->version) != 3)
        return -1;

    if(strncmp(req->url, "http://", 7) == 0){
        char *host_start = req->url + 7;
        char *slash = strchr(host_start, '/');
        char *colon;

        if(slash){
            strncpy(req->path, slash, sizeof(req->path) - 1);
            *slash = '\0';
        }else{
            strcpy(req->path, "/");
        }

        colon = strchr(host_start, ':');
        if(colon){
            req->port = atoi(colon + 1);
            *colon = '\0';
        }

        strncpy(req->host, host_start, sizeof(req->host) - 1);
    }else if(strcmp(req->method, "CONNECT") == 0){
        char *colon = strchr(req->url, ':');
        if(colon){
            req->port = atoi(colon + 1);
            *colon = '\0';
        }
        strncpy(req->host, req->url, sizeof(req->host) - 1);
        req->port = req->port ? req->port : 443;
    }else{
        strncpy(req->path, req->url, sizeof(req->path) - 1);
    }

    char *ptr = line_end + 2;
    while(req->header_count < MAX_HEADERS){
        char *end = strstr(ptr, "\r\n");
        if(!end || end == ptr) break;
        *end = '\0';

        char *colon = strchr(ptr, ':');
        if (colon) {
            *colon = '\0';
            strncpy(req->headers[req->header_count][0], ptr, 255);
            // skip leading space after colon
            char *val = colon + 1;
            while (*val == ' ') val++;
            strncpy(req->headers[req->header_count][1], val, 255);

            // Extract Host header if we didn't get host from URL
            if (strcasecmp(req->headers[req->header_count][0], "host") == 0
                && req->host[0] == '\0') {
                strncpy(req->host, req->headers[req->header_count][1], 255);
            }
            req->header_count++;
        }
        ptr = end + 2;
    }
    return 0;
}

void print_request(const HttpRequest *req) {
    printf("=== Parsed HTTP Request ===\n");
    printf("Method  : %s\n", req->method);
    printf("Host    : %s\n", req->host);
    printf("Port    : %d\n", req->port);
    printf("Path    : %s\n", req->path);
    printf("Version : %s\n", req->version);
    printf("Headers : %d\n", req->header_count);
    for (int i = 0; i < req->header_count; i++)
        printf("  [%s]: %s\n", req->headers[i][0], req->headers[i][1]);
    printf("===========================\n");
}

int main(){
    int server_fd, client_fd;
    struct sockaddr_in addr;
    socklen_t client_len = sizeof(addr);
    char buffer[BUFFER];
    int opt = 1;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0){perror("socket"); exit(1);}

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if(bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0){perror("bind");exit(1);}

    if(listen(server_fd, 10) < 0){perror("listen"); exit(1);}

    printf("[Server] listening to port %d...\n", PORT);
    while(1){
        client_fd = accept(server_fd, (struct sockaddr*)&addr, &client_len);
        if(client_fd < 0){perror("accept"); continue;}

        // printf("[connected] %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

        ssize_t n;
        n = read_http_headers(client_fd, buffer, BUFFER);
        
        // while((n = recv(client_fd, buffer, sizeof(buffer)-1, 0)) > 0){
        //     buffer[n] = '\0';
        //     printf("[recv] %zd bytes: %s", n, buffer);
        //     send(client_fd, buffer, n, 0);
        // }
        if(n > 0){
            HttpRequest req;
            if(parseREQ(buffer, &req) == 0){
                print_request(&req);
                const char *resp = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK";
                send(client_fd, resp, strlen(resp), 0);
            }
        }
        printf("[disconnected]\n");
        close(client_fd);
    }
//    close(server_fd);
   return 0; 
}