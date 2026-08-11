#ifndef XPS_UTILS_H
#define XPS_UTILS_H

#include "../xps.h"

bool is_valid_port(u_int port);
/**
 * Validates if the given port is within the valid TCP port range (1 - 65535).
 */

int make_socket_non_blocking(u_int sock_fd);
/**
 * Sets a socket file descriptor to non-blocking mode using fcntl.
 * Returns 0 on success, or -1 on failure.
 */

struct addrinfo *xps_getaddrinfo(const char *host, u_int port);
/**
 * Resolves host and port into a struct addrinfo for socket binding.
 */

char *get_remote_ip(int sock_fd);
/**
 * Retrieves the IP address string of a connected client socket.
 * Note: Returned pointer is dynamically allocated with malloc and must be freed.
 */

#endif