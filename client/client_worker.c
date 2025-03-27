// client_worker.c
#include "client_worker.h"
#include <stdio.h>      // For fprintf, perror, printf
#include <stdlib.h>     // For malloc, free
#include <string.h>     // For strerror, memset
#include <errno.h>      // For errno
#include <unistd.h>     // For close

// ********************************* //

// Creates and initializes a ClientWorker structure.
client_worker_t *client_worker_create() {
    client_worker_t *worker = malloc(sizeof(client_worker_t));
    if (!worker) {
        perror("client_worker_create: malloc failed");
        return NULL;
    }

    worker->connection = NULL;
    memset(&worker->server_address, 0, sizeof(worker->server_address));

    return worker;
}

// ********************************* //

// Deletes and frees resources associated with the ClientWorker.
void client_worker_delete(client_worker_t *worker) {
    if (!worker) {
        return; // Safe to call with NULL
    }

    client_worker_disconnect(worker); // Disconnect if still connected

    free(worker); // Free the worker structure itself
}

// ********************************* //

// Connects the ClientWorker to a server at the specified address and port.
client_worker_status_t client_worker_connect(client_worker_t *worker, in_addr_t server_addr, in_port_t server_port) {
    if (!worker) {
        fprintf(stderr, "client_worker_connect: Null worker argument\n");
        return CLIENT_WORKER_STATUS_ERROR_NULL_POINTER;
    }

    if (worker->connection) {
        fprintf(stderr, "client_worker_connect: Already connected, disconnect first\n");
        return CLIENT_WORKER_STATUS_ERROR_SOCKET_OPERATION; // Or a more specific status if needed
    }

    udp_socket_t *udp_socket = udp_socket_create(INADDR_ANY, 0); // Client socket binds to any available port
    if (!udp_socket) {
        fprintf(stderr, "client_worker_connect: Failed to create UDP socket\n");
        return CLIENT_WORKER_STATUS_ERROR_CREATE_CONNECTION;
    }

    message_queue_t *msg_queue = message_queue_create(128); // Client side queue, adjust size as needed
    if (!msg_queue) {
        fprintf(stderr, "client_worker_connect: Failed to create message queue\n");
        udp_socket_delete(udp_socket);
        return CLIENT_WORKER_STATUS_ERROR_CREATE_CONNECTION;
    }

    net_connection_t *conn = net_connection_create(OWNER_CLIENT, udp_socket, msg_queue);
    if (!conn) {
        fprintf(stderr, "client_worker_connect: Failed to create net connection\n");
        message_queue_delete(msg_queue);
        udp_socket_delete(udp_socket);
        return CLIENT_WORKER_STATUS_ERROR_CREATE_CONNECTION;
    }
    worker->connection = conn;

    // Set server address for sending messages
    worker->server_address.sin_family = AF_INET;
    worker->server_address.sin_port = server_port;
    worker->server_address.sin_addr.s_addr = server_addr;
    worker->connection->socket->client_addr = worker->server_address; // Set client_addr in socket for send_to_client to work

    return CLIENT_WORKER_STATUS_SUCCESS;
}

// ********************************* //

// Sends a message to the connected server.
client_worker_status_t client_worker_send(client_worker_t *worker, const message_t *message) {
    if (!worker) {
        fprintf(stderr, "client_worker_send: Null worker argument\n");
        return CLIENT_WORKER_STATUS_ERROR_NULL_POINTER;
    }
    if (!worker->connection) {
        fprintf(stderr, "client_worker_send: Not connected to server\n");
        return CLIENT_WORKER_STATUS_ERROR_SOCKET_OPERATION;
    }
    if (!message) {
        fprintf(stderr, "client_worker_send: Null message argument\n");
        return CLIENT_WORKER_STATUS_ERROR_NULL_POINTER;
    }

    // Before sending, ensure the socket's client_addr is set to the server's address
    worker->connection->socket->client_addr = worker->server_address;

    net_connection_status_t send_status = net_connection_send(worker->connection, message);
    if (send_status != NET_CONNECTION_STATUS_SUCCESS) {
        fprintf(stderr, "client_worker_send: Error sending message: %d\n", send_status);
        return CLIENT_WORKER_STATUS_ERROR_SEND_MESSAGE;
    }

    return CLIENT_WORKER_STATUS_SUCCESS;
}

// ********************************* //

// Receives a message from the connected server (blocking operation).
message_t *client_worker_receive(client_worker_t *worker) {
    if (!worker) {
        fprintf(stderr, "client_worker_receive: Null worker argument\n");
        return NULL;
    }
    if (!worker->connection) {
        fprintf(stderr, "client_worker_receive: Not connected to server\n");
        return NULL;
    }

    return net_connection_receive(worker->connection); // Use blocking receive
}

// ********************************* //

// Disconnects the ClientWorker from the server and releases resources.
client_worker_status_t client_worker_disconnect(client_worker_t *worker) {
    if (!worker) {
        fprintf(stderr, "client_worker_disconnect: Null worker argument\n");
        return CLIENT_WORKER_STATUS_ERROR_NULL_POINTER;
    }

    if (worker->connection) {
        net_connection_delete(worker->connection);
        worker->connection = NULL;
    }

    return CLIENT_WORKER_STATUS_SUCCESS;
}

// ********************************* //