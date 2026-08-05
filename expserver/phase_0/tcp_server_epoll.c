#include<arpa/inet.h>
#include<netdb.h>
#include<netinet/in.h>

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include<sys/socket.h>
#include<sys/epoll.h>

#include<unistd.h>

#define PORT 8080
#define BUFF_SIZE 10000
#define MAX_ACCEPPT_BACKLOG 5
#define MAX_EPOLL_EVENTS 10

void strrev(char *str) {
  for (int start=0, end=strlen(str)-2; start<end; start++, end--) {
    char temp = str[start];
    str[start] = str[end];
    str[end] = temp;
  }
}


int main(){
        //same as stage 1 tcp server
    int listen_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    int enable = 1;
    setsockopt (listen_sock_fd,SOL_SOCKET,SO_REUSEADDR, &enable, sizeof(int));
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);
    bind(listen_sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(listen_sock_fd, MAX_ACCEPPT_BACKLOG);
    printf("!!! SERVER LISTENING TO PORT %d\n", PORT);
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
        //until here
    int epoll_fd = epoll_create1(0);
    
}
