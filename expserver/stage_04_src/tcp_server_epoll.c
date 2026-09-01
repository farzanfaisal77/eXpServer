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
  
  //to monitor specific fds and letting us know connections on it
  //we need to use event and events data structures under struct epoll event
  struct epoll_event event, events[MAX_EPOLL_EVENTS];
  // now we want to monitor our listening socket so
  event.events = EPOLLIN;
  event.data.fd = listen_sock_fd;
  //adding this to our monitor list
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_sock_fd, &event);

  //so far we created an epoll instance 
  // and we added this to oour "monitoring" list
  while(1){
	printf("[DEBUG] Epoll wait\n");
    int n_ready_fds= epoll_wait(epoll_fd, events,MAX_EPOLL_EVENTS,-1);
    for(int i=0; i<n_ready_fds; i++){
      int curr_fd = events[i].data.fd;
      //accept client connection and add to epoll here
      if (curr_fd == listen_sock_fd){
        int conn_sock_fd = accept (listen_sock_fd, (struct sockaddr*)&client_addr, &client_addr_len);
        if(conn_sock_fd != -1){
          event.events = EPOLLIN;
          event.data.fd = conn_sock_fd;
          epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_sock_fd, &event);
        }
      }
      else{ /* event on connection socket */
        char buff[BUFF_SIZE];
        memset(buff,0,BUFF_SIZE);
        //read data coming from client into the buffer
        ssize_t read_n= recv(curr_fd, buff, sizeof(buff), 0);
        //handing all read cases
        if(read_n<0){
            printf("!!! NO DATA WAS READ, ERROR OCCURED, CLOSING SERVER !!!\n");
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, curr_fd, NULL);
			close(curr_fd);
        }
        else if(read_n==0){
            printf("!!! THE CLIENT with FD %d DISCONNECTED !!!\n", curr_fd);
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, curr_fd, NULL);
			close(curr_fd);
        }
		else{
			printf("CLIENT_MESSAGE: %s", buff);

			strrev(buff);

			//send reversed string to client
			send(curr_fd, buff, read_n, 0);
		}
      }
    }
  }

}
