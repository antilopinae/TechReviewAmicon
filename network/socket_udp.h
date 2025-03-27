#ifndef SOCKET_UDP_H
#define SOCKET_UDP_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#define UDP_SOCKET_BUFFER_SIZE 2000

typedef void (*udp_socket_receive_callback_t)(struct sockaddr_in client_addr, char* client_message, ssize_t message_len);

typedef enum
{
    UDP_SOCKET_STATUS_SUCCESS = 0,
    UDP_SOCKET_STATUS_ERROR_CREATE_SOCKET,
    UDP_SOCKET_STATUS_ERROR_BIND,
    UDP_SOCKET_STATUS_ERROR_RECEIVE,
    UDP_SOCKET_STATUS_ERROR_SEND,
    UDP_SOCKET_STATUS_ERROR_MEMORY
} udp_socket_status_t;

typedef struct udp_socket
{
    int fd;
    struct sockaddr_in outcoming_addr;
    struct sockaddr_in incoming_addr;
    socklen_t incoming_addr_len;
    char incoming_buffer[UDP_SOCKET_BUFFER_SIZE];
    char outcoming_buffer[UDP_SOCKET_BUFFER_SIZE];
    int receive_timeout_sec;
} udp_socket_t;

// Creates and initializes a UDP socket
udp_socket_t* udp_socket_create(in_addr_t addr, in_port_t port);

// Frees up the resources allocated for the UDP socket, closes it if necessary
void udp_socket_delete(udp_socket_t* socket);

// Binds the socket to the specified address and port
udp_socket_status_t udp_socket_bind(udp_socket_t* socket);

// Accepts a message (blocking operation) and calls the callback function
udp_socket_status_t udp_socket_receive_message(udp_socket_t* socket, udp_socket_receive_callback_t receive_callback);

// Sends a message to owner
udp_socket_status_t udp_socket_send(udp_socket_t* socket, size_t data_length);

// Set timeout for receiving
udp_socket_status_t udp_socket_set_receive_timeout(udp_socket_t* socket, int seconds);

#endif    // SOCKET_UDP_H