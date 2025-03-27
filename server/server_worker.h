#ifndef SERVER_WORKER_H
#define SERVER_WORKER_H

#include <netinet/in.h>
#include <pthread.h>

#include "net_connection.h"
#include "socket_udp.h"

#define SERVER_RECEIVE_TIMEOUT_SECONDS 1
#define SERVER_SEND_TIMEOUT_SECONDS 1
#define SERVER_MESSAGE_QUEUE_SIZE 128

typedef enum server_worker_status
{
    SERVER_WORKER_STATUS_SUCCESS = 0,
    SERVER_WORKER_STATUS_ERROR_NULL_POINTER,
    SERVER_WORKER_STATUS_ERROR_CREATE_CONNECTION,
    SERVER_WORKER_STATUS_ERROR_START_THREADS,
    SERVER_WORKER_STATUS_ERROR_SOCKET_OPERATION,
    SERVER_WORKER_STATUS_ERROR_THREAD_OPERATION
} server_worker_status_t;

typedef struct server_worker
{
    net_connection_t* connection;
    void (*worker_function)(message_t*);
    int is_running;
} server_worker_t;

// Creates and initializes the ServerWorker structure
server_worker_t* server_worker_create(in_addr_t addr, in_port_t port);

// Destroys the ServerWorker structure and releases the allocated resources
void server_worker_delete(server_worker_t* worker);

// Starts the ServerWorker worker threads (network and message processing)
server_worker_status_t server_worker_start(server_worker_t* worker);

// Stops the ServerWorker worker threads
server_worker_status_t server_worker_stop(server_worker_t* worker);

// Set receiver on message in worker
void server_worker_set_on_message(server_worker_t* worker, void (*worker_function)(message_t*));

// Is waiting for the first message = incoming connection
void server_worker_wait_for_connection(server_worker_t* worker);

// Sends a message over the connection
server_worker_status_t server_worker_send_message(server_worker_t* worker, message_t* message);

// Updates the status of the worker by processing incoming messages from the queue
void server_worker_update(server_worker_t* worker, size_t max_messages);

// Message processing function
void server_worker_on_message(server_worker_t* worker, message_t* message);

#endif    // SERVER_WORKER_H