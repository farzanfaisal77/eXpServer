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
int route_table[MAX_SOCKS][2], route_table_size = 0; //Define MAX_SOCKS=10 as a global variable

int connect_upstream() {
  /* connect to upstream server */
}

void accept_connection(int listen_sock_fd) {
  /* accept client connection */
}

void handle_client(int conn_sock_fd) {
  /* handle client */
}

void handle_upstream(int upstream_sock_fd) {
  /* handle upstream */
}

int create_loop() {
    /* return new epoll instance */
}

void loop_attach(int epoll_fd, int fd, int events) {
    /* attach fd to epoll */
}

int create_server() {
    /* create listening socket and return it */
}

void loop_run(int epoll_fd) {
    /* infinite loop and processing epoll events */
}

int main() {
  /* initialize proxy */

  //listen_sock_fd = /* create server using server_create() */

  //epoll_fd = /* create loop instance using loop_create() */

  /* attach server to event loop using loop_attach() */

  /* start event loop with loop_run() */
}