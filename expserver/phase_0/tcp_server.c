#include<arpa/inet.h>
#include<netdb.h>
#include<netinet/in.h>

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include<sys/socket.h>

#include<unistd.h>

#define PORT 8080
#define BUFF_SIZE 10000
#define MAX_ACCEPPT_BACKLOG 5


int main(){
    // creating a listening socket
    int listen_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    // AF_INET - iPv4
    // SOCK_STREAM - Stream socket, TCP
    // 0 - default protocol

    int enable = 1;
    setsockopt (listen_sock_fd,SOL_SOCKET,SO_REUSEADDR, &enable, sizeof(int));
}