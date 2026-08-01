#include<arpa/inet.h>
#include<netdb.h>
#include<netinet/in.h>

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include<sys/socket.h>

#include<unistd.h>
#include<pthread.h>

#define PORT 8080
#define BUFF_SIZE 10000
#define MAX_ACCEPPT_BACKLOG 5

int main(){
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    int enable = 1;
    setsockopt (sockfd,SOL_SOCKET,SO_REUSEADDR, &enable, sizeof(int));
    
    struct sockaddr_in serveraddr;

    //setting up server address
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); //assings IP Address. INADDR_ANY represents any available IP on host device
    //htonl() host byte order to network byte order
    serveraddr.sin_port = htons(PORT); // assigning port
    //htons same function as htonl

    bind(sockfd, (struct sockaddr_in*) &serveraddr, sizeof(serveraddr));

    //create client socket (???)
    struct sockaddr_in clientaddr;
    int len=0;
    //(???)
    //now that the socket is binded we can start recieving
    //for udp we use recvfrom
    char buffer[BUFF_SIZE];
    recvfrom(sockfd, buffer, BUFF_SIZE, 0, (struct sockaddr*)& clientaddr, &len);
}