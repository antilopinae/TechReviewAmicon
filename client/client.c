#include "client.h"

#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "message.h"
#include "utility.h"

client_context_t* client_create_context(client_mode_t mode, in_addr_t server_addr, int port_count, const int* ports_arr)
{
    if (!server_addr || port_count <= 0 || !ports_arr)
    {
        DEBUG_LOG("client_create_context: null args");
        return NULL;
    }

    client_context_t* context = malloc(sizeof(client_context_t));
    if (!context)
    {
        DEBUG_LOG("client_create_context: malloc failed");
        return NULL;
    }
    memset(context, 0, sizeof(client_context_t));

    context->mode = mode;

    context->port_count = port_count;
    context->is_running = 0;

    context->server_sockaddrs = malloc(context->port_count * sizeof(struct sockaddr_in));
    context->connections = malloc(sizeof(net_connection_t) * port_count);

    if (!context->server_sockaddrs)
    {
        DEBUG_LOG("client_create_context: malloc failed");
        SAFE_FREE(context);
        return NULL;
    }

    context->connections = malloc(sizeof(net_connection_t) * port_count);
    if (!context->connections)
    {
        DEBUG_LOG("client_create_context: malloc failed");
        SAFE_FREE(context->server_sockaddrs);
        SAFE_FREE(context);
        return NULL;
    }

    message_queue_t *incoming_queue, *outgoing_queue;
    udp_socket_t* socket;

    // TODO: More complex logic for errors (not so much time)
    for (int i = 0; i < port_count; i++)
    {
        socket = udp_socket_create(inet_addr("127.0.0.1"), 0);
        if (!socket)
        {
            DEBUG_LOG("client_start: udp_socket_create failed");
            goto client_context_socket_create_failed;
        }
        if (udp_socket_bind(socket) != UDP_SOCKET_STATUS_SUCCESS)
        {
            DEBUG_LOG("client_start: udp_socket_bind failed");
            goto client_context_socket_bind_failed;
        }

        context->server_sockaddrs[i].sin_family = AF_INET;
        context->server_sockaddrs[i].sin_addr.s_addr = server_addr;
        context->server_sockaddrs[i].sin_port = htons(ports_arr[i]);

        incoming_queue = message_queue_create(5);
        outgoing_queue = message_queue_create(5);

        if (!incoming_queue || !outgoing_queue)
        {
            DEBUG_LOG("client_start: message_queue_create failed");
            goto client_context_queue_create_failed;
        }

        net_connection_t* connection = net_connection_create(socket, incoming_queue, outgoing_queue, 1, 1);
        if (!connection)
        {
            DEBUG_LOG("client_start: net_connection_create failed");
            goto client_context_queue_create_failed;
        }
        context->connections[i] = connection;
    }

    return context;

client_context_queue_create_failed:
    message_queue_delete(incoming_queue);
    message_queue_delete(outgoing_queue);

client_context_socket_bind_failed:
    udp_socket_delete(socket);

client_context_socket_create_failed:
    SAFE_FREE(context->server_sockaddrs);
    SAFE_FREE(context->connections);
    SAFE_FREE(context);

    return NULL;
}

void client_delete_context(client_context_t* context)
{
    if (!context)
    {
        DEBUG_LOG("client_delete_context: NULL context");
        return;
    }
    client_stop(context);

    for (int i = 0; i < context->port_count; i++)
    {
        net_connection_delete(context->connections[i]);
    }

    SAFE_FREE(context->server_sockaddrs);
    SAFE_FREE(context->connections);

    SAFE_FREE(context);
}

client_status_t client_start(client_context_t* context)
{
    if (!context)
    {
        return CLIENT_STATUS_ERROR_NULL_POINTER;
    }

    if (context->is_running)
    {
        return CLIENT_STATUS_SUCCESS;
    }

    for (int i = 0; i < context->port_count; i++)
    {
        if (net_connection_start_receiving(context->connections[i]) != NET_CONNECTION_STATUS_SUCCESS)
        {
            DEBUG_LOG("client_start: net_connection_start_receiving failed");
            return CLIENT_STATUS_ERROR_START_THREADS;
        }
        if (net_connection_start_sending(context->connections[i]) != NET_CONNECTION_STATUS_SUCCESS)
        {
            DEBUG_LOG("client_start: net_connection_start_sending failed");
            return CLIENT_STATUS_ERROR_START_THREADS;
        }
    }

    context->is_running = 1;
    return CLIENT_STATUS_SUCCESS;
}

client_status_t client_stop(client_context_t* context)
{
    if (!context)
    {
        return CLIENT_STATUS_ERROR_NULL_POINTER;
    }
    if (!context->is_running)
    {
        return CLIENT_STATUS_SUCCESS;
    }

    for (int i = 0; i < context->port_count; i++)
    {
        net_connection_stop_receiving(context->connections[i]);
        net_connection_stop_sending(context->connections[i]);
    }

    context->is_running = 0;

    return CLIENT_STATUS_SUCCESS;
}

client_status_t client_send_pid_message(client_context_t* context, pid_t pid)
{
    if (!context)
    {
        return CLIENT_STATUS_ERROR_NULL_POINTER;
    }

    for (int i = 0; i < context->port_count && (i == 0 || context->mode == CLIENT_MODE_FUZZ); i++)
    {
        message_t* message = message_create(MESSAGE_TYPE_PID, sizeof(pid_t));
        if (!message)
        {
            DEBUG_LOG("client_send_pid_message: message_create failed");
            return CLIENT_STATUS_ERROR_MEMORY_ALLOC;
        }
        message->header.type = MESSAGE_TYPE_PID;
        message->header.length = sizeof(pid_t);
        message->header.owner_addr = context->server_sockaddrs[i];

        message_write(message, (const uint8_t*)&pid, sizeof(pid_t));

        net_connection_status_t send_status = net_connection_send(context->connections[i], message);
        if (send_status != NET_CONNECTION_STATUS_SUCCESS)
        {
            DEBUG_LOG("client_send_pid_message: net_connection_send failed: %d", send_status);
            return CLIENT_STATUS_ERROR_SEND_MESSAGE;
        }
    }

    sleep(1);

    return CLIENT_STATUS_SUCCESS;
}

void client_print_config(const char* server_addr, client_mode_t mode, int port_count, const int* ports, int pid_count, const int* pids)
{
    printf("Server Address: %s\n", server_addr);
    printf("Mode: %s\n", mode == CLIENT_MODE_FUZZ ? "fuzz" : "nfuzz");
    printf("Ports count: %d\n", port_count);
    printf("Ports: ");
    for (int i = 0; i < port_count; i++)
    {
        printf("%d ", ports[i]);
    }
    printf("\n");
    printf("Pids: ");
    for (int i = 0; i < pid_count; i++)
    {
        printf("%d ", pids[i]);
    }
    printf("\n");
}

void* print_received_messages(void* arg)
{
    client_context_t* context = (client_context_t*)arg;

    while (1)
    {
        for (int i = 0; i < context->port_count; i++)
        {
            message_t* message = net_connection_receive_nonblocking(context->connections[i]);
            if (message)
            {
                printf("Received message: %s\n", (const char*)message->data);
            }
        }
        sleep(1);
    }
}

// "Usage: %s <server_addr> <mode(fuzz|nfuzz)> <count_ports> <ports...> <count_pids> <pids...>\n"
int client(int argc, char* argv[])
{
    const char* server_addr = SERVER_ADDR;
    client_mode_t mode = CLIENT_MODE;
    int port_count = SERVER_NUM_PORT;
    int ports[SERVER_NUM_PORT] = SERVER_PORT_ARRAY;
    int pid_count = CLIENT_NUM_PID;
    int pids[CLIENT_NUM_PID] = CLIENT_PID_ARRAY;

    client_print_config(server_addr, mode, port_count, ports, pid_count, pids);

    client_context_t* context = client_create_context(mode, inet_addr(server_addr), port_count, ports);

    if (!context || client_start(context) != CLIENT_STATUS_SUCCESS)
    {
        fprintf(stderr, "Client context creation failed.\n");
        goto client_cleanup_context;
    }

    pthread_t thread;
    // Thread for printing messages
    if (pthread_create(&thread, NULL, print_received_messages, context) != 0)
    {
        perror("Error create thread");
        return 1;
    }

    switch (context->mode)
    {
        case CLIENT_MODE_NORMAL:
        {
            if (client_send_pid_message(context, pids[0]) != CLIENT_STATUS_SUCCESS)
            {
                fprintf(stderr, "Client context sending failed.\n");
                goto client_cleanup_context;
            }
            sleep(1);    // Wait for incoming messages to print them
            break;
        }
        case CLIENT_MODE_FUZZ:
        {
            while (1)
            {
                for (int i = 0; i < pid_count; i++)
                {
                    if (client_send_pid_message(context, pids[i]) != CLIENT_STATUS_SUCCESS)
                    {
                        fprintf(stderr, "Client context sending failed.\n");
                        goto client_cleanup_context;
                    }
                }
            }
            break;
        }
    }

client_cleanup_context:
    if (context)
    {
        client_delete_context(context);
    }

client_cleanup_pids:
    // SAFE_FREE(pids);

client_cleanup_ports:
    // SAFE_FREE(ports);

    return EXIT_SUCCESS;
}
