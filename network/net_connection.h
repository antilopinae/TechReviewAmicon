#ifndef CONNECTION_H
#define CONNECTION_H

#include <pthread.h>

#include "message_queue.h"
#include "socket_udp.h"
#include "utility.h"

typedef struct udp_socket udp_socket_t;
typedef struct message_queue message_queue_t;

typedef enum net_connection_status
{
    NET_CONNECTION_STATUS_SUCCESS = 0,
    NET_CONNECTION_STATUS_ERROR_NULL_POINTER,
    NET_CONNECTION_STATUS_ERROR_SOCKET,
    NET_CONNECTION_STATUS_ERROR_QUEUE,
    NET_CONNECTION_STATUS_ERROR_THREAD,
    NET_CONNECTION_STATUS_ERROR_MESSAGE_FORMAT
} net_connection_status_t;

typedef struct net_connection
{
    udp_socket_t* socket;

    message_queue_t* incoming_message_queue;
    message_queue_t* outgoing_message_queue;

    pthread_t receive_thread;
    pthread_t send_thread;

    int is_sending;
    int is_receiving;

    int receive_timeout_seconds;
    int send_timeout_seconds;
} net_connection_t;

// Flow function for receiving messages
void* net_connection_receive_thread_func(void* arg);

// Flow function for sending messages
void* net_connection_send_thread_func(void* arg);

// Creates and initializes a network connection.
// Returns a pointer to the created connection or NULL in case of an error.
net_connection_t* net_connection_create(
    udp_socket_t* socket, message_queue_t* incoming_message_queue, message_queue_t* outgoing_message_queue, int receive_timeout_seconds, int send_timeout_seconds);

// Destroys the network connection, stops the receiving stream and releases the allocated resources.
void net_connection_delete(net_connection_t* connection);

// Starts the process of receiving messages in a separate thread.
net_connection_status_t net_connection_start_receiving(net_connection_t* connection);

// Stops the flow of receiving messages.
net_connection_status_t net_connection_stop_receiving(net_connection_t* connection);

// Starts the process of sending messages in a separate thread.
net_connection_status_t net_connection_start_sending(net_connection_t* connection);

// Stops the flow of sending messages.
net_connection_status_t net_connection_stop_sending(net_connection_t* connection);

// Sends the message through the queue.
net_connection_status_t net_connection_send(net_connection_t* connection, message_t* message);

// Accepts an incoming message from the message queue (blocking operation).
message_t* net_connection_receive(net_connection_t* connection);

// Accepts an incoming message from the message queue (non-blocking operation).
message_t* net_connection_receive_nonblocking(net_connection_t* connection);

// Reads the message header from the socket
static net_connection_status_t net_connection_read_header(net_connection_t* connection, message_header_t* header);

// Reads the message data from the socket
static net_connection_status_t net_connection_read_data(net_connection_t* connection, message_t* message);

// Writes the message header to the socket
static net_connection_status_t net_connection_write_header(net_connection_t* connection, const message_header_t* header);

// Writes the message data to the socket
static net_connection_status_t net_connection_write_data(net_connection_t* connection, const message_t* message);

// Adds the received message to the incoming message queue
net_connection_status_t net_connection_add_to_queue(net_connection_t* connection, message_queue_t* message_queue, message_t* message);

#endif    // CONNECTION_H