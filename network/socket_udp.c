#include "socket_udp.h"

#include "utility.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
        SAFE_FREE(usocket);
        return NULL;
    }

    memset(&usocket->outcoming_buffer, 0, sizeof(usocket->outcoming_buffer));
    memset(&usocket->incoming_buffer, 0, sizeof(usocket->incoming_buffer));

    usocket->outcoming_addr.sin_family = AF_INET;
    usocket->outcoming_addr.sin_port = port;
    usocket->outcoming_addr.sin_addr.s_addr = addr;

    usocket->incoming_addr_len = sizeof(struct sockaddr_in);
    usocket->receive_timeout_sec = 0;

    return usocket;
}

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

udp_socket_status_t udp_socket_bind(udp_socket_t* usocket)
{
    if (!usocket)
    {
        DEBUG_LOG("Invalid socket pointer\n");
        return UDP_SOCKET_STATUS_ERROR_BIND;
    }

    if (bind(usocket->fd, (struct sockaddr*)&usocket->outcoming_addr, sizeof(usocket->outcoming_addr)) == -1)
    {
        DEBUG_LOG("Bind error: %s (errno: %d)\n", strerror(errno), errno);
        return UDP_SOCKET_STATUS_ERROR_BIND;
    }

    // DEBUG_LOG("Socket bind successful\n");
    return UDP_SOCKET_STATUS_SUCCESS;
}

udp_socket_status_t udp_socket_receive_message(udp_socket_t* usocket, udp_socket_receive_callback_t receive_callback)
{
    if (!usocket)
    {
        DEBUG_LOG("Invalid socket pointer\n");
        return UDP_SOCKET_STATUS_ERROR_RECEIVE;
    }

    // DEBUG_LOG("Listening for incoming messages...\n");

    ssize_t recv_len = recvfrom(usocket->fd, usocket->incoming_buffer, sizeof(usocket->incoming_buffer), 0, (struct sockaddr*)&usocket->incoming_addr, &usocket->incoming_addr_len);
    if (recv_len == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // DEBUG_LOG("Receive timeout occurred\n");
            return UDP_SOCKET_STATUS_ERROR_RECEIVE;
        }
        else
        {
            DEBUG_LOG("Receive error: %s\n", strerror(errno));
            return UDP_SOCKET_STATUS_ERROR_RECEIVE;
        }
    }

    // DEBUG_LOG("Received message from IP: %s and port: %i\n", inet_ntoa(usocket->incoming_addr.sin_addr), ntohs(usocket->incoming_addr.sin_port));

    if (receive_callback)
    {
        receive_callback(usocket->incoming_addr, usocket->incoming_buffer, recv_len);
    }

    return UDP_SOCKET_STATUS_SUCCESS;
}

udp_socket_status_t udp_socket_send(udp_socket_t* usocket, size_t data_length)
{
    if (!usocket)
    {
        DEBUG_LOG("Invalid socket pointer\n");
        return UDP_SOCKET_STATUS_ERROR_SEND;
    }

    // DEBUG_LOG("udp_socket_send: Sending data, length: %zu\n", data_length);
    // DEBUG_LOG("udp_socket_send: Outgoing buffer (first 8 bytes before sendto): %02X %02X %02X %02X %02X %02X %02X %02X\n", usocket->outcoming_buffer[0],
    // usocket->outcoming_buffer[1], usocket->outcoming_buffer[2], usocket->outcoming_buffer[3], usocket->outcoming_buffer[4], usocket->outcoming_buffer[5],
    // usocket->outcoming_buffer[6], usocket->outcoming_buffer[7]);

    ssize_t sent_len = sendto(usocket->fd, usocket->outcoming_buffer, data_length, 0, (struct sockaddr*)&usocket->incoming_addr, usocket->incoming_addr_len);
    if (sent_len == -1)
    {
        DEBUG_LOG("Send error\n");
        return UDP_SOCKET_STATUS_ERROR_SEND;
    }

    return UDP_SOCKET_STATUS_SUCCESS;
}

udp_socket_status_t udp_socket_set_receive_timeout(udp_socket_t* socket, int seconds)
{
    if (!socket)
    {
        DEBUG_LOG("Invalid socket pointer\n");
        return UDP_SOCKET_STATUS_ERROR_CREATE_SOCKET;
    }

    struct timeval tv;
    tv.tv_sec = seconds;
    tv.tv_usec = 0;

    if (setsockopt(socket->fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) < 0)
    {
        DEBUG_LOG("Error setting socket receive timeout: %s\n", strerror(errno));
        return UDP_SOCKET_STATUS_ERROR_CREATE_SOCKET;
    }

    socket->receive_timeout_sec = seconds;
    return UDP_SOCKET_STATUS_SUCCESS;
}
