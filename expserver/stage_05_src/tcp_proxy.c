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
#define UPSTREAM_PORT 3000
#define MAX_SOCKS 10

int listen_sock_fd, epoll_fd;
struct epoll_event events[MAX_EPOLL_EVENTS];
int route_table[MAX_SOCKS][2],route_table_size = 0;

int connect_upstream() {

  int upstream_sock_fd=socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in upstream_addr;
  upstream_addr.sin_family = AF_INET;
  upstream_addr.sin_port = htons(UPSTREAM_PORT);
  upstream_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  connect(upstream_sock_fd, (struct sockaddr*)&upstream_addr, sizeof(upstream_addr));

  return upstream_sock_fd;

}

void loop_attach(int epoll_fd, int fd, int events) {
  struct epoll_event event;
  event.events = events;
  event.data.fd = fd;
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
}

void accept_connection(int listen_sock_fd) {

  struct sockaddr_in client_addr;
  socklen_t client_addr_len= sizeof(client_addr);
  int conn_sock_fd = accept(listen_sock_fd, (struct sockaddr*)&client_addr, &client_addr_len);

  if (conn_sock_fd==-1) return;

  /* add conn_sock_fd to loop using loop_attach() */
  loop_attach(epoll_fd, conn_sock_fd, EPOLLIN);

  //create connection to upstream server
  int upstream_sock_fd= connect_upstream();

  /* add upstream_sock_fd to loop using loop_attach() */
  loop_attach(epoll_fd, upstream_sock_fd, EPOLLIN);

  // add conn_sock_fd and upstream_sock_fd to routing table
  route_table[route_table_size][0]= conn_sock_fd;
  route_table[route_table_size][1]= upstream_sock_fd;
  route_table_size += 1;
}

void handle_client(int conn_sock_fd) {

  char buff[BUFF_SIZE];
  memset(buff, 0, BUFF_SIZE);

  int read_n = recv(conn_sock_fd, buff, sizeof(buff), 0);

  //client closed connection or error occurred
  if (read_n<=0) {
    if (read_n<0) {
      printf("!!! NO DATA WAS READ, ERROR OCCURED, CLOSING SERVER !!!\n");
    }
    else {
        printf("!!! THE CLIENT with FD %d DISCONNECTED !!!\n", conn_sock_fd);
    }
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, conn_sock_fd, NULL);
    close(conn_sock_fd);
    return;
  }

  /* print client message (helpful for Milestone #2) */
  printf("CLIENT_MESSAGE: %s", buff);

  /* find the right upstream socket from the route table */
  int upstream_sock_fd = -1;
  for (int i = 0; i < route_table_size; i++) {
    if (route_table[i][0] == conn_sock_fd) {
      upstream_sock_fd = route_table[i][1];
      break;
    }
  }

  if (upstream_sock_fd!=-1){
    // sending client message to upstream
    int bytes_written = 0;
    int message_len = read_n;
    while (bytes_written < message_len) {
      int n = send(upstream_sock_fd, buff + bytes_written, message_len - bytes_written, 0);
      if (n <= 0) break;
      bytes_written += n;
    }
  }

}

void handle_upstream(int upstream_sock_fd) {

  char buff[BUFF_SIZE];
  memset(buff,0,BUFF_SIZE);

  int read_n = recv(upstream_sock_fd, buff, sizeof(buff), 0);

  //Upstream closed connection or error occurred
  if (read_n<=0) {
    if (read_n < 0) {
      printf("!!! NO DATA WAS READ FROM UPSTREAM, ERROR OCCURED !!!\n");
    }
    else{
      printf("!!! UPSTREAM SERVER with FD %d DISCONNECTED !!!\n", upstream_sock_fd);
    }
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, upstream_sock_fd, NULL);
    close(upstream_sock_fd);
    return;
  }

  /* find the right client socket from the route table */
  int conn_sock_fd = -1;
  for (int i = 0; i < route_table_size; i++) {
    if (route_table[i][1] == upstream_sock_fd) {
      conn_sock_fd = route_table[i][0];
      break;
    }
  }

  /* send upstream message to client */
  if (conn_sock_fd != -1) {
    int bytes_written = 0;
    int message_len=read_n;
    while (bytes_written < message_len) {
      int n=send(conn_sock_fd, buff+bytes_written, message_len-bytes_written, 0);
      if (n<=0) break;
      bytes_written += n;
    }
  }

}

int create_loop() {
  /* return new epoll instance */
  return epoll_create1(0);
}

int create_server() {
  /* create listening socket and return it */
  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  int enable = 1;
  setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(PORT);

  bind(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
  listen(sock_fd, MAX_ACCEPPT_BACKLOG);
  printf("!!! SERVER LISTENING TO PORT %d\n", PORT);

  return sock_fd;
}

void loop_run(int epoll_fd) { //infinite loop processing epoll events
  while (1) {
    printf("[DEBUG] Epoll wait\n");
    int n_ready=epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, -1);
    for (int i=0; i<n_ready; i++) {
      int curr_fd = events[i].data.fd;

      if (curr_fd == listen_sock_fd) {
        accept_connection(listen_sock_fd);
      }
      else{
        // check if event is on client socket or upstream socket
        int is_client = 0;
        int is_upstream = 0;

        for (int r = 0; r < route_table_size; r++) {
          if(route_table[r][0] == curr_fd) {
            is_client = 1;
            break;
          }
          if(route_table[r][1] == curr_fd) {
            is_upstream = 1;
            break;
          }
        }
        if(is_client) {
          handle_client(curr_fd);
        }
        else if(is_upstream) {
          handle_upstream(curr_fd);
        }
      }
    }

  }
}

int main() {
  /* initialize proxy */
  listen_sock_fd=create_server();

  epoll_fd=create_loop();

  /* attach server to event loop using loop_attach() */
  loop_attach(epoll_fd, listen_sock_fd, EPOLLIN);

  /* start event loop with loop_run() */
  loop_run(epoll_fd);

  return 0;
}