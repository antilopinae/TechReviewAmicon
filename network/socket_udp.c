#include "socket_udp.h"

#include "utility.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// ********************************* //

udp_socket_t* udp_socket_create(in_addr_t addr, in_port_t port)
{
    udp_socket_t* usocket = (udp_socket_t*)malloc(sizeof(udp_socket_t));
    if (!usocket)
    {
        DEBUG_LOG("Failed to allocate memory for socket\n");
        return NULL;
    }

    usocket->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (usocket->fd == -1)
    {
        DEBUG_LOG("Failed to create socket: %s\n");
        free(usocket);
        return NULL;
    }
    DEBUG_LOG("Socket created successfully\n");

    memset(&usocket->outcoming_buffer, 0, sizeof(usocket->outcoming_buffer));

    usocket->outcoming_addr.sin_family = AF_INET;
    usocket->outcoming_addr.sin_port = port;
    usocket->outcoming_addr.sin_addr.s_addr = addr;

    usocket->incoming_addr_len = sizeof(usocket->incoming_addr);

    return usocket;
}

// ********************************* //

void udp_socket_delete(udp_socket_t* usocket)
{
    if (usocket)
    {
        if (usocket->fd != -1)
        {
            close(usocket->fd);
        }
        SAFE_FREE(usocket);
    }
}

// ********************************* //

udp_socket_status_t udp_socket_bind(udp_socket_t* usocket)
{
    if (!usocket)
    {
        DEBUG_LOG("Invalid socket pointer\n");
        return UDP_SOCKET_STATUS_ERROR_BIND;
    }

    if (bind(usocket->fd, (struct sockaddr*)&usocket->outcoming_addr, sizeof(usocket->outcoming_addr)) == -1)
    {
        DEBUG_LOG("Bind error: %s\n", strerror(errno));
        return UDP_SOCKET_STATUS_ERROR_BIND;
    }

    DEBUG_LOG("Socket bind successful\n");
    return UDP_SOCKET_STATUS_SUCCESS;
}

// ********************************* //

udp_socket_status_t udp_socket_receive_message(udp_socket_t* usocket, udp_socket_receive_callback_t receive_callback)
{
    if (!usocket)
    {
        DEBUG_LOG("Invalid socket pointer\n");
        return UDP_SOCKET_STATUS_ERROR_RECEIVE;
    }

    DEBUG_LOG("Listening for incoming messages...\n");

    ssize_t recv_len = recvfrom(usocket->fd, usocket->incoming_buffer, UDP_SOCKET_BUFFER_SIZE - 1, 0, (struct sockaddr*)&usocket->incoming_addr, &usocket->incoming_addr_len);
    if (recv_len == -1)
    {
        DEBUG_LOG("Receive error: %s\n", strerror(errno));
        return UDP_SOCKET_STATUS_ERROR_RECEIVE;
    }

    if (receive_callback)
    {
        receive_callback(usocket->incoming_addr, usocket->incoming_buffer, recv_len);
    }

    return UDP_SOCKET_STATUS_SUCCESS;
}

// ********************************* //

udp_socket_status_t udp_socket_send_to(udp_socket_t* usocket)
{
    if (!usocket)
    {
        DEBUG_LOG("Invalid socket pointer\n");
        return UDP_SOCKET_STATUS_ERROR_SEND;
    }

    ssize_t sent_len = sendto(usocket->fd, usocket->incoming_buffer, strlen(usocket->incoming_buffer), 0, (struct sockaddr*)&usocket->incoming_addr, usocket->incoming_addr_len);
    if (sent_len == -1)
    {
        DEBUG_LOG("Send error\n");
        return UDP_SOCKET_STATUS_ERROR_SEND;
    }

    return UDP_SOCKET_STATUS_SUCCESS;
}

// ********************************* //