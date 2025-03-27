#include "server_worker.h"

#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utility.h"

server_worker_t* server_worker_create(in_addr_t addr, in_port_t port)
{
    server_worker_t* worker = malloc(sizeof(server_worker_t));
    if (!worker)
    {
        DEBUG_LOG("server_worker_create: malloc failed");
        return NULL;
    }
    memset(worker, 0, sizeof(server_worker_t));

    worker->connection = NULL;
    worker->worker_function = NULL;

    udp_socket_t* udp_socket = udp_socket_create(addr, htons(port));
    if (!udp_socket)
    {
        DEBUG_LOG("server_worker_create: Failed to create UDP socket");
        goto server_worker_socket_create_failed;
    }

    udp_socket_status_t bind_status = udp_socket_bind(udp_socket);
    if (bind_status != UDP_SOCKET_STATUS_SUCCESS)
    {
        DEBUG_LOG("server_worker_create: UDP socket bind failed: %d", bind_status);
        goto server_worker_socket_bind_failed;
    }

    message_queue_t* incoming_queue = message_queue_create(SERVER_MESSAGE_QUEUE_SIZE);
    message_queue_t* outgoing_queue = message_queue_create(SERVER_MESSAGE_QUEUE_SIZE);

    if (!incoming_queue || !outgoing_queue)
    {
        DEBUG_LOG("server_worker_create: Failed to create message queue");
        goto server_worker_socket_bind_failed;
    }

    net_connection_t* conn = net_connection_create(udp_socket, incoming_queue, outgoing_queue, SERVER_RECEIVE_TIMEOUT_SECONDS, SERVER_SEND_TIMEOUT_SECONDS);
    if (!conn)
    {
        DEBUG_LOG("server_worker_create: Failed to create net connection");
        goto server_worker_queue_create_failed;
    }

    worker->connection = conn;

    return worker;

server_worker_queue_create_failed:
    message_queue_delete(incoming_queue);
    message_queue_delete(outgoing_queue);

server_worker_socket_bind_failed:
    udp_socket_delete(udp_socket);

server_worker_socket_create_failed:
    SAFE_FREE(worker);

    return NULL;
}

void server_worker_delete(server_worker_t* worker)
{
    if (!worker)
    {
        return;
    }

    server_worker_stop(worker);

    if (worker->connection)
    {
        net_connection_delete(worker->connection);
        worker->connection = NULL;
    }

    SAFE_FREE(worker);
}

server_worker_status_t server_worker_start(server_worker_t* worker)
{
    if (!worker)
    {
        DEBUG_LOG("server_worker_start: Null worker argument");
        return SERVER_WORKER_STATUS_ERROR_NULL_POINTER;
    }

    if (worker->is_running)
    {
        DEBUG_LOG("server_worker_start: Worker already running");
        return SERVER_WORKER_STATUS_SUCCESS;
    }

    worker->is_running = 1;

    net_connection_status_t net_status_recv = net_connection_start_receiving(worker->connection);
    if (net_status_recv != NET_CONNECTION_STATUS_SUCCESS)
    {
        DEBUG_LOG("server_worker_start: Failed to start receiving thread: %d", net_status_recv);
        worker->is_running = 0;
        return SERVER_WORKER_STATUS_ERROR_START_THREADS;
    }

    net_connection_status_t net_status_send = net_connection_start_sending(worker->connection);
    if (net_status_send != NET_CONNECTION_STATUS_SUCCESS)
    {
        DEBUG_LOG("server_worker_start: Failed to start sending thread: %d", net_status_send);
        worker->is_running = 0;
        return SERVER_WORKER_STATUS_ERROR_START_THREADS;
    }

    return SERVER_WORKER_STATUS_SUCCESS;
}

server_worker_status_t server_worker_stop(server_worker_t* worker)
{
    if (!worker)
    {
        DEBUG_LOG("server_worker_stop: Null worker argument");
        return SERVER_WORKER_STATUS_ERROR_NULL_POINTER;
    }

    if (!worker->is_running)
    {
        DEBUG_LOG("server_worker_stop: Worker not running");
        return SERVER_WORKER_STATUS_SUCCESS;
    }

    worker->is_running = 0;
    net_connection_status_t stop_recv_status = net_connection_stop_receiving(worker->connection);
    if (stop_recv_status != NET_CONNECTION_STATUS_SUCCESS)
    {
        DEBUG_LOG("server_worker_stop: Error stopping receiving thread: %d", stop_recv_status);
        // TODO: More complex logic
    }

    net_connection_status_t stop_send_status = net_connection_stop_sending(worker->connection);
    if (stop_send_status != NET_CONNECTION_STATUS_SUCCESS)
    {
        DEBUG_LOG("server_worker_stop: Error stopping receiving thread: %d", stop_send_status);
        // TODO: More complex logic
    }

    return SERVER_WORKER_STATUS_SUCCESS;
}

void server_worker_set_on_message(server_worker_t* worker, void (*worker_function)(message_t*))
{
    worker->worker_function = worker_function;
}

void server_worker_wait_for_connection(server_worker_t* worker)
{
    if (!worker)
    {
        DEBUG_LOG("server_worker_wait_for_connection: Null worker argument\n");
        return;
    }

    DEBUG_LOG("server_worker_wait_for_connection: Waiting for the first message...\n");

    while (worker->is_running)
    {
        message_t* message = net_connection_receive_nonblocking(worker->connection);
        if (message)
        {
            DEBUG_LOG("server_worker_wait_for_connection: First message received. Connection established in UDP context.");
            net_connection_add_to_queue(worker->connection, worker->connection->incoming_message_queue, message);
            break;
        }
        sleep(SERVER_RECEIVE_TIMEOUT_SECONDS);
    }

    if (!worker->is_running)
    {
        DEBUG_LOG("server_worker_wait_for_connection: Wait for connection interrupted because worker is stopping.");
    }
}

server_worker_status_t server_worker_send_message(server_worker_t* worker, message_t* message)
{
    if (!worker || !message || !worker->connection)
    {
        DEBUG_LOG("server_worker_send_message: Null argument(s)");
        return SERVER_WORKER_STATUS_ERROR_NULL_POINTER;
    }
    // TODO: Replace SERVER_WORKER_STATUS with CONNECTION_STATUS
    DEBUG_LOG("server_worker_send_message: Sending message of type %d, length %d to IP: %s, port: %d\n", message->header.type, message->header.length,
        inet_ntoa(message->header.owner_addr.sin_addr), ntohs(message->header.owner_addr.sin_port));

    return net_connection_send(worker->connection, message);
}

void server_worker_update(server_worker_t* worker, size_t max_messages)
{
    if (!worker)
    {
        DEBUG_LOG("server_worker_update: Null worker argument");
        return;
    }

    size_t message_count = 0;
    while (message_count < max_messages && worker->is_running)
    {
        message_t* message = net_connection_receive_nonblocking(worker->connection);
        if (message)
        {
            server_worker_on_message(worker, message);
            server_worker_send_message(worker, message);
            message_count++;
        }
    }
}

void server_worker_on_message(server_worker_t* worker, message_t* message)
{
    if (!worker || !message)
    {
        DEBUG_LOG("server_worker_on_message: Null argument(s)\n");
        return;
    }

    struct sockaddr_in* client_addr = &message->header.owner_addr;
    if (client_addr)
    {
        DEBUG_LOG("Received message from IP: %s and port: %i\n", inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));
    }

    if (worker->worker_function)
    {
        worker->worker_function(message);
    }

    DEBUG_LOG("Message from client: %.*s\n", message->header.length, (char*)message->data);
}
