#ifndef CLIENT_H
#define CLIENT_H

#include <pthread.h>

#include "net_connection.h"

typedef enum client_mode
{
    CLIENT_MODE_NORMAL,
    CLIENT_MODE_FUZZ
} client_mode_t;

typedef enum client_status
{
    CLIENT_STATUS_SUCCESS = 0,
    CLIENT_STATUS_ERROR_NULL_POINTER,
    CLIENT_STATUS_ERROR_CREATE_CONNECTION,
    CLIENT_STATUS_ERROR_START_THREADS,
    CLIENT_STATUS_ERROR_SEND_MESSAGE,
    CLIENT_STATUS_ERROR_RECEIVE_MESSAGE,
    CLIENT_STATUS_ERROR_INVALID_ARGUMENT,
    CLIENT_STATUS_ERROR_THREAD_OPERATION,
    CLIENT_STATUS_ERROR_MEMORY_ALLOC
} client_status_t;

typedef struct client_context
{
    net_connection_t** connections;
    struct sockaddr_in* server_sockaddrs;
    client_mode_t mode;
    int port_count;
    int is_running;
} client_context_t;

// Function to create client context
client_context_t* client_create_context(client_mode_t mode, in_addr_t server_addr, int ports_count, const int* ports_arr);

// Function to delete client context
void client_delete_context(client_context_t* context);

// Function to start client (start sending)
client_status_t client_start(client_context_t* context);

// Function to stop client (stop sending in fuzzing mode)
client_status_t client_stop(client_context_t* context);

// Function to send a message (PID) to server
client_status_t client_send_pid_message(client_context_t* context, pid_t pid);

#endif    // CLIENT_H