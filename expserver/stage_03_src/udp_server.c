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

typedef struct {
    char message[BUFF_SIZE];
    struct sockaddr_in client_addr;
    int sockfd;
    socklen_t addr_len;
} client_data_t;

void strrev(char *str) {
  for (int start=0, end=strlen(str)-2; start<end; start++, end--) {
    char temp = str[start];
    str[start] = str[end];
    str[end] = temp;
  }
}

void* handle_client(void *arg){
    client_data_t* data = (client_data_t*) arg;
    printf("!!! CLIENT MESSAGE !!! : %s", data->message);
    strrev(data->message);
    
    sendto(data->sockfd, data->message, strlen(data->message), 0, (struct sockaddr*)(&data->client_addr), data->addr_len);

    free(data);
    pthread_exit(NULL);
}

int main(){
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    
    struct sockaddr_in serveraddr;
    //setting up server address
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); //assings IP Address. INADDR_ANY finds any available IP on host device
    //htonl() host byte order to network byte order
    serveraddr.sin_port = htons(PORT); // assigning port
    //htons same function as htonl

    bind(sockfd, (struct sockaddr*) &serveraddr, sizeof(serveraddr));
    printf("!!! SERVER LISTENING TO PORT %d\n", PORT);
    while(1){
        //client socket details structure
        struct sockaddr_in clientaddr;
        socklen_t len = sizeof(clientaddr);
        //for udp we use recvfrom
        char buffer[BUFF_SIZE];
        ssize_t n = recvfrom(sockfd, buffer, BUFF_SIZE, 0, (struct sockaddr*)& clientaddr, &len);
        buffer[n]='\0';
        client_data_t* data = (client_data_t*)malloc(sizeof(client_data_t));
            strcpy(data->message, buffer);
            data->client_addr = clientaddr;
            data->sockfd = sockfd;
            data->addr_len = len;

        pthread_t thread_id;
        if(pthread_create(&thread_id, NULL, handle_client, (void*)data) != 0){
            perror("Failed to create thread");
            free(data);
        }
        pthread_detach(thread_id);
    }
    close(sockfd);
}