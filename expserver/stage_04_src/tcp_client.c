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

int main(){
    //making client socket
    int client_sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    //setting server details to connect to server
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr =inet_addr("127.0.0.1");

    //connecting to server
    if(connect(client_sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0){
        printf("!!! CONNECTION FAILED, ABORTING !!!\n");
        exit(1);
    }
    else{
        printf("!!! SUCCESS! CONNECTED TO TCP SERVER !!!\n");
    }
    while(1){
        char* line=NULL;
        //line_len is space that is malloced by getline after it reas the line. this will be space occupied by line var
        //read_n is actual number of bytes read
        size_t line_len=0, read_n=0;
        printf("ENTER SERVER MESSAGE: ");
        read_n = getline(&line, &line_len, stdin);
        //send the read line to the server. whatever the client socket gets is forwarded to the server kernel because of the connect() we did above
        send(client_sock_fd, line, read_n, 0);
        char buff[BUFF_SIZE];
        memset(buff,0,BUFF_SIZE);
        //recieving what the server kernel send to the client socket
        read_n = recv(client_sock_fd, buff,BUFF_SIZE, 0);
        if(read_n <=0 ){
            printf("!!! CLIENT CLOSED / ERROR RECIEVING MESSAGE !!!");
            exit(1);
        }

        printf("SERVER MESSAGE: %s", buff);
    }
    return 0;
}