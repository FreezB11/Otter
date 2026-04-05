///@file: proxy.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define BUFFER 4096

int main(){
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER];
    int opt = 1;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0){perror("socket"); exit(1);}

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if(bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){perror("bind");exit(1);}

    if(listen(server_fd, 10) < 0){perror("listen"); exit(1);}

    printf("[Server] listening to port %d...\n", PORT);
    while(1){
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if(client_fd < 0){perror("accept"); continue;}

        printf("[connected] %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        ssize_t n;
        while((n = recv(client_fd, buffer, sizeof(buffer)-1, 0)) > 0){
            buffer[n] = '\0';
            printf("[recv] %zd bytes: %s", n, buffer);
            send(client_fd, buffer, n, 0);
        }
        printf("[disconnected]\n");
        close(client_fd);
    }
   close(server_fd);
   return 0; 
}