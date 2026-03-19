///@file:dummy
// this is justa dummy server that willl be sending request like a normal application
// i will send random thing first then a http one 
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

int main(){

    int fd = socket(SOCK_STREAM, INADDR_ANY, 0);
    if(fd == -1){
        printf("sorry mate you have socket failure!!!!!!\n");
    }
    
    return 0;
}