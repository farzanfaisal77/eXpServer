#include<bits/stdc++.h>
using namespace std;

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

// Function to reverse a string in-place
void strrev(char *str) {
  for (int start = 0, end = strlen(str) - 2; start < end; start++, end--) {
    char temp = str[start];
    str[start] = str[end];
    str[end] = temp;
  }
}


int main(){
    // creating a listening socket
    int listen_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    // AF_INET - iPv4
    // SOCK_STREAM - Stream socket, TCP
    // 0 - default protocol

    //to bind the socket to same IP and Port while restarting
    int enable = 1;
    setsockopt (listen_sock_fd,SOL_SOCKET,SO_REUSEADDR, &enable, sizeof(int));

    struct sockaddr_in server_addr;

    //setting up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); //assings IP Address. INADDR_ANY represents any available IP on host device
    //htonl() host byte order to network byte order
    server_addr.sin_port = htons(PORT); // assigning port
    //htons same function as htonl

    //now we need to bind the listening socket to specified port and ip
    bind(listen_sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));

    //now we need to listen to the port
    listen(listen_sock_fd, MAX_ACCEPPT_BACKLOG);
    cout<<"!!! SERVER LISTENING TO PORT "<<PORT<<"\n";

    //client connection and client connection details
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;

    //accepting connection
    int conn_sock_fd = accept (listen_sock_fd, (struct sockaddr*)&client_addr, &client_addr_len);
    //prgm will block here until a client has connected
    cout<<"WOW BRO CLIENT CONNECTED!!!\n";
    while(1){
        char buff[BUFF_SIZE];
        memset(buff,0,BUFF_SIZE);
        //read data coming from client into the buffer
        ssize_t read_n= recv(conn_sock_fd, buff, sizeof(buff), 0);
        //handing all read cases
        if(read_n<0){
            cout<<"!!! NO DATA WAS READ, ERROR OCCURED, CLOSING SERVER !!!\n";
            close(conn_sock_fd);
            exit(1);
        }
        if(read_n==0){
            cout<<"!!! THE CLIENT HAS DISCONNECTED, CLOSING SERVER !!!\n";
            close(conn_sock_fd);
            exit(1);
        }

        cout<<"CLIENT_MESSAGE: "<<buff<<"\n";

        strrev(buff);

        //send reversed string to client
        send(conn_sock_fd, buff, read_n, 0);
    }
}
