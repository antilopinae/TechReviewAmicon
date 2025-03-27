// client_worker.h
#ifndef CLIENT_WORKER_H
#define CLIENT_WORKER_H

#include "net_connection.h" // For net_connection_t, net_connection_status_t
#include "socket_udp.h"     // For udp_socket_t, udp_socket_status_t
#include <netinet/in.h>     // For in_addr_t, in_port_t

// ********************************* //

// Forward declarations
typedef struct net_connection net_connection_t;

// ********************************* //

// Status codes for ClientWorker operations
typedef enum client_worker_status {
    CLIENT_WORKER_STATUS_SUCCESS = 0,
    CLIENT_WORKER_STATUS_ERROR_NULL_POINTER,
    CLIENT_WORKER_STATUS_ERROR_CREATE_CONNECTION,
    CLIENT_WORKER_STATUS_ERROR_SOCKET_OPERATION,
    CLIENT_WORKER_STATUS_ERROR_SEND_MESSAGE,
    CLIENT_WORKER_STATUS_ERROR_DISCONNECT
} client_worker_status_t;

// ********************************* //

// Structure representing the ClientWorker
typedef struct client_worker {
    net_connection_t *connection; // Network connection for the client
    struct sockaddr_in server_address; // Server address to connect to
} client_worker_t;

// ********************************* //

// Creates and initializes a ClientWorker structure.
// Returns a pointer to the created structure, or NULL on failure.
client_worker_t *client_worker_create();

// Deletes and frees resources associated with the ClientWorker.
void client_worker_delete(client_worker_t *worker);

// Connects the ClientWorker to a server at the specified address and port.
client_worker_status_t client_worker_connect(client_worker_t *worker, in_addr_t server_addr, in_port_t server_port);

// Sends a message to the connected server.
client_worker_status_t client_worker_send(client_worker_t *worker, const message_t *message);

// Receives a message from the connected server (blocking operation).
message_t *client_worker_receive(client_worker_t *worker);

// Disconnects the ClientWorker from the server and releases resources.
client_worker_status_t client_worker_disconnect(client_worker_t *worker);

// ********************************* //

#endif // CLIENT_WORKER_H