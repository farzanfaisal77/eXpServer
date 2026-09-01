#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define SERVER_PORT 8080
#define SERVER_ADDR "127.0.0.1"
#define BUFF_SIZE 10000
#define NUM_CLIENTS 3

void client_process(int client_id) {
    int client_sock_fd;
    struct sockaddr_in server_addr;

    // Create client socket
    client_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock_fd < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);
    server_addr.sin_port = htons(SERVER_PORT);

    // Connect to TCP server
    if (connect(client_sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
        printf("[Client %d] [ERROR] Failed to connect to server\n", client_id);
        exit(EXIT_FAILURE);
    } else {
        printf("[Client %d] [INFO] Connected to server\n", client_id);
    }

    char *line = NULL;
    size_t line_len = 0;

    while (1) {
        printf("[Client %d] Enter message: ", client_id);
        fflush(stdout);

        ssize_t read_n = getline(&line, &line_len, stdin);

        if (read_n <= 0) {
            printf("\n[Client %d] Exiting...\n", client_id);
            break;
        }

        send(client_sock_fd, line, read_n, 0);

        char buff[BUFF_SIZE];
        memset(buff, 0, BUFF_SIZE);

        ssize_t recv_n = recv(client_sock_fd, buff, sizeof(buff) - 1, 0);

        if (recv_n <= 0) {
            printf("[Client %d] [INFO] Server disconnected. Closing client\n", client_id);
            break;
        }

        printf("[Client %d] [SERVER MESSAGE] %s", client_id, buff);
        break;
    }

    free(line);
    close(client_sock_fd);
}

int main() {
    // Fork and run clients strictly one after another
    for (int i = 0; i < NUM_CLIENTS; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process runs the client
            client_process(i + 1);
            exit(0);
        }

        // Parent waits for current child to finish before starting the next
        wait(NULL);
    }

    return 0;
}