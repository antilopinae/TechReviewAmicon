#include "net_connection.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void* net_connection_receive_thread_func(void* arg)
{
    net_connection_t* connection = (net_connection_t*)arg;
    if (!connection)
    {
        DEBUG_LOG("net_connection_receive_thread_func: Null connection argument\n");
        return NULL;
    }

    while (connection->is_receiving)
    {
        message_t* received_message = NULL;
        udp_socket_status_t socket_status;

        socket_status = udp_socket_receive_message(connection->socket, NULL);
        if (socket_status != UDP_SOCKET_STATUS_SUCCESS)
        {
            // DEBUG_LOG("Continue receiving: %d\n", socket_status);
            continue;
        }

        // Allocate memory for message header and data
        received_message = message_create(MESSAGE_TYPE_NONE, UDP_SOCKET_BUFFER_SIZE);
        if (!received_message)
        {
            DEBUG_LOG("net_connection_receive_thread_func: Failed to create message\n");
            continue;    // Try next loop
        }

        // Read header from socket's received buffer
        net_connection_status_t header_status = net_connection_read_header(connection, &received_message->header);
        received_message->header.owner_addr = connection->socket->incoming_addr;

        if (header_status != NET_CONNECTION_STATUS_SUCCESS)
        {
            DEBUG_LOG("net_connection_receive_thread_func: Error reading message header: %d\n", header_status);
            received_message->header.type = MESSAGE_TYPE_NONE;
            goto net_connection_add;
        }

        // Read data from socket's received buffer
        net_connection_status_t data_status = net_connection_read_data(connection, received_message);
        if (data_status != NET_CONNECTION_STATUS_SUCCESS)
        {
            DEBUG_LOG("net_connection_receive_thread_func: Error reading message data: %d\n", data_status);
            received_message->header.type = MESSAGE_TYPE_NONE;
            goto net_connection_add;
        }

        // DEBUG_LOG("net_connection_receive_thread_func: Header and data read successfully. Type: %d, Length: %d, Data: %s",
        // received_message->header.type,received_message->header.length, (const char*)received_message->data);

    net_connection_add:
        // Add the received message to the incoming queue
        net_connection_status_t queue_status = net_connection_add_to_queue(connection, connection->incoming_message_queue, received_message);
        if (queue_status != NET_CONNECTION_STATUS_SUCCESS)
        {
            DEBUG_LOG("net_connection_receive_thread_func: Error adding message to queue: %d\n", queue_status);
            message_delete(received_message);
            continue;
        }
    }

    return NULL;
}

void* net_connection_send_thread_func(void* arg)
{
    net_connection_t* connection = (net_connection_t*)arg;
    if (!connection)
    {
        DEBUG_LOG("net_connection_send_thread_func: Null connection argument\n");
        return NULL;
    }

    while (connection->is_sending)
    {
        message_t* sending_message = message_queue_dequeue_nonblocking(connection->outgoing_message_queue);
        if (!sending_message)
        {
            // DEBUG_LOG("Continue sending messages\n");
            sleep(connection->receive_timeout_seconds);
            continue;
        }

        net_connection_status_t header_status = net_connection_write_header(connection, &sending_message->header);
        if (header_status != NET_CONNECTION_STATUS_SUCCESS)
        {
            DEBUG_LOG("net_connection_send: Error writing header: %d\n", header_status);
            message_delete(sending_message);
            continue;
        }

        net_connection_status_t data_status = net_connection_write_data(connection, sending_message);
        if (data_status != NET_CONNECTION_STATUS_SUCCESS)
        {
            DEBUG_LOG("net_connection_send: Error writing data: %d\n", data_status);
            message_delete(sending_message);
            continue;
        }

        connection->socket->incoming_addr = sending_message->header.owner_addr;

        // DEBUG_LOG("net_connection_send_thread_func: Sending message. Type: %d, Length: %d\n", sending_message->header.type, sending_message->header.length);
        // DEBUG_LOG("net_connection_send_thread_func: Outgoing buffer (first 8 bytes before send): %02X %02X %02X %02X %02X %02X %02X %02X\n",
        //     connection->socket->outcoming_buffer[0], connection->socket->outcoming_buffer[1], connection->socket->outcoming_buffer[2], connection->socket->outcoming_buffer[3],
        //     connection->socket->outcoming_buffer[4], connection->socket->outcoming_buffer[5], connection->socket->outcoming_buffer[6], connection->socket->outcoming_buffer[7]);

        udp_socket_status_t send_status = udp_socket_send(connection->socket, sending_message->header.length + sizeof(message_header_t));

        if (send_status != UDP_SOCKET_STATUS_SUCCESS)
        {
            DEBUG_LOG("net_connection_send: Error sending via UDP socket: %d\n", send_status);
        }

        message_delete(sending_message);
    }

    return NULL;
}

net_connection_t* net_connection_create(
    udp_socket_t* socket, message_queue_t* incoming_message_queue, message_queue_t* outgoing_message_queue, int receive_timeout_seconds, int send_timeout_seconds)
{
    if (!socket || !incoming_message_queue || !outgoing_message_queue || receive_timeout_seconds < 0 || send_timeout_seconds < 0)
    {
        DEBUG_LOG("net_connection_create: Null socket or queue argument\n");
        return NULL;
    }

    net_connection_t* connection = malloc(sizeof(net_connection_t));
    if (!connection)
    {
        DEBUG_LOG("net_connection_create: malloc failed");
        return NULL;
    }

    connection->socket = socket;
    connection->incoming_message_queue = incoming_message_queue;
    connection->outgoing_message_queue = outgoing_message_queue;
    connection->is_receiving = 0;
    connection->is_sending = 0;

    connection->receive_timeout_seconds = receive_timeout_seconds;
    connection->send_timeout_seconds = send_timeout_seconds;

    udp_socket_set_receive_timeout(socket, receive_timeout_seconds);

    return connection;
}

void net_connection_delete(net_connection_t* connection)
{
    if (!connection)
    {
        return;
    }

    net_connection_stop_receiving(connection);
    net_connection_stop_sending(connection);

    if (connection->socket)
    {
        udp_socket_delete(connection->socket);
        connection->socket = NULL;
    }
    if (connection->incoming_message_queue)
    {
        message_queue_delete(connection->incoming_message_queue);
        connection->incoming_message_queue = NULL;
    }
    if (connection->outgoing_message_queue)
    {
        message_queue_delete(connection->outgoing_message_queue);
        connection->outgoing_message_queue = NULL;
    }

    SAFE_FREE(connection);
}

net_connection_status_t net_connection_start_receiving(net_connection_t* connection)
{
    if (!connection)
    {
        DEBUG_LOG("net_connection_start_receiving: Null connection argument\n");
        return NET_CONNECTION_STATUS_ERROR_NULL_POINTER;
    }

    if (connection->is_receiving)
    {
        DEBUG_LOG("net_connection_start_receiving: Receiving thread already running\n");
        return NET_CONNECTION_STATUS_SUCCESS;
    }

    connection->is_receiving = 1;
    int result = pthread_create(&connection->receive_thread, NULL, net_connection_receive_thread_func, connection);
    if (result != 0)
    {
        DEBUG_LOG("net_connection_start_receiving: pthread_create failed: %s\n", strerror(result));
        connection->is_receiving = 0;
        return NET_CONNECTION_STATUS_ERROR_THREAD;
    }

    return NET_CONNECTION_STATUS_SUCCESS;
}

net_connection_status_t net_connection_stop_receiving(net_connection_t* connection)
{
    if (!connection)
    {
        DEBUG_LOG("net_connection_stop_receiving: Null connection argument\n");
        return NET_CONNECTION_STATUS_ERROR_NULL_POINTER;
    }

    if (!connection->is_receiving)
    {
        // DEBUG_LOG("net_connection_stop_receiving: Receiving thread not running\n");
        return NET_CONNECTION_STATUS_SUCCESS;
    }

    connection->is_receiving = 0;

    if (pthread_join(connection->receive_thread, NULL) != 0)
    {
        DEBUG_LOG("net_connection_stop_receiving: pthread_join failed");
        return NET_CONNECTION_STATUS_ERROR_THREAD;
    }

    return NET_CONNECTION_STATUS_SUCCESS;
}

net_connection_status_t net_connection_start_sending(net_connection_t* connection)
{
    if (!connection)
    {
        DEBUG_LOG("net_connection_start_sending: Null connection argument\n");
        return NET_CONNECTION_STATUS_ERROR_NULL_POINTER;
    }

    if (connection->is_sending)
    {
        DEBUG_LOG("net_connection_start_sending: Receiving thread already running\n");
        return NET_CONNECTION_STATUS_SUCCESS;
    }

    connection->is_sending = 1;
    int result = pthread_create(&connection->send_thread, NULL, net_connection_send_thread_func, connection);
    if (result != 0)
    {
        DEBUG_LOG("net_connection_start_receiving: pthread_create failed: %s\n", strerror(result));
        connection->is_sending = 0;
        return NET_CONNECTION_STATUS_ERROR_THREAD;
    }

    return NET_CONNECTION_STATUS_SUCCESS;
}

net_connection_status_t net_connection_stop_sending(net_connection_t* connection)
{
    if (!connection)
    {
        DEBUG_LOG("net_connection_stop_sending: Null connection argument\n");
        return NET_CONNECTION_STATUS_ERROR_NULL_POINTER;
    }

    if (!connection->is_sending)
    {
        // DEBUG_LOG("net_connection_stop_sending: Sending thread not running\n");
        return NET_CONNECTION_STATUS_SUCCESS;
    }

    connection->is_sending = 0;

    // pthread_cancel(connection->send_thread);

    if (pthread_join(connection->send_thread, NULL) != 0)
    {
        DEBUG_LOG("net_connection_stop_sending: pthread_join failed");
        return NET_CONNECTION_STATUS_ERROR_THREAD;
    }

    return NET_CONNECTION_STATUS_SUCCESS;
}

net_connection_status_t net_connection_send(net_connection_t* connection, message_t* message)
{
    if (!connection || !message || !connection->socket || !connection->outgoing_message_queue)
    {
        DEBUG_LOG("net_connection_send: Null argument(s)\n");
        return NET_CONNECTION_STATUS_ERROR_NULL_POINTER;
    }

    return net_connection_add_to_queue(connection, connection->outgoing_message_queue, message);
}

message_t* net_connection_receive(net_connection_t* connection)
{
    if (!connection || !connection->incoming_message_queue)
    {
        DEBUG_LOG("net_connection_receive: Null argument(s)\n");
        return NULL;
    }
    return message_queue_dequeue(connection->incoming_message_queue);
}

message_t* net_connection_receive_nonblocking(net_connection_t* connection)
{
    if (!connection || !connection->incoming_message_queue)
    {
        DEBUG_LOG("net_connection_receive_nonblocking: Null argument(s)\n");
        return NULL;
    }
    return message_queue_dequeue_nonblocking(connection->incoming_message_queue);
}

static net_connection_status_t net_connection_read_header(net_connection_t* connection, message_header_t* header)
{
    if (!connection || !header || !connection->socket)
    {
        DEBUG_LOG("net_connection_read_header: Null argument(s)\n");
        return NET_CONNECTION_STATUS_ERROR_NULL_POINTER;
    }

    memcpy(header, connection->socket->incoming_buffer, sizeof(message_header_t));

    // TODO: add validation logic here if needed...

    return NET_CONNECTION_STATUS_SUCCESS;
}

static net_connection_status_t net_connection_read_data(net_connection_t* connection, message_t* message)
{
    if (!connection || !message || !connection->socket)
    {
        DEBUG_LOG("net_connection_read_data: Null argument(s)\n");
        return NET_CONNECTION_STATUS_ERROR_NULL_POINTER;
    }

    if (message->header.length > 0)
    {
        if (message->header.length > UDP_SOCKET_BUFFER_SIZE - sizeof(message_header_t))
        {
            DEBUG_LOG("net_connection_read_data: Message length exceeds buffer capacity\n");
            return NET_CONNECTION_STATUS_ERROR_MESSAGE_FORMAT;
        }
        memcpy(message->data, connection->socket->incoming_buffer + sizeof(message_header_t), message->header.length);
    }
    return NET_CONNECTION_STATUS_SUCCESS;
}

static net_connection_status_t net_connection_write_header(net_connection_t* connection, const message_header_t* header)
{
    if (!connection || !header || !connection->socket)
    {
        DEBUG_LOG("net_connection_write_header: Null argument(s)\n");
        return NET_CONNECTION_STATUS_ERROR_NULL_POINTER;
    }

    // DEBUG_LOG("net_connection_write_header: Writing header. Type: %d, Length: %d\n", header->type, header->length);

    memcpy(connection->socket->outcoming_buffer, header, sizeof(message_header_t));
    // DEBUG_LOG("net_connection_write_header: Header bytes written to buffer: %02X %02X %02X %02X %02X %02X %02X %02X\n", connection->socket->outcoming_buffer[0],
    //     connection->socket->outcoming_buffer[1], connection->socket->outcoming_buffer[2], connection->socket->outcoming_buffer[3], connection->socket->outcoming_buffer[4],
    //     connection->socket->outcoming_buffer[5], connection->socket->outcoming_buffer[6], connection->socket->outcoming_buffer[7]);

    return NET_CONNECTION_STATUS_SUCCESS;
}

static net_connection_status_t net_connection_write_data(net_connection_t* connection, const message_t* message)
{
    if (!connection || !message || !connection->socket)
    {
        DEBUG_LOG("net_connection_write_data: Null argument(s)\n");
        return NET_CONNECTION_STATUS_ERROR_NULL_POINTER;
    }

    if (message->header.length > 0)
    {
        if (message->header.length > UDP_SOCKET_BUFFER_SIZE - sizeof(message_header_t))
        {
            DEBUG_LOG("net_connection_write_data: Message length exceeds buffer capacity\n");
            return NET_CONNECTION_STATUS_ERROR_MESSAGE_FORMAT;
        }
        memcpy(connection->socket->outcoming_buffer + sizeof(message_header_t), message->data, message->header.length);
        // DEBUG_LOG("net_connection_write_data: Data bytes written to buffer: %02X %02X %02X %02X %02X %02X %02X %02X\n",
        //     connection->socket->outcoming_buffer[sizeof(message_header_t) + 0], connection->socket->outcoming_buffer[sizeof(message_header_t) + 1],
        //     connection->socket->outcoming_buffer[sizeof(message_header_t) + 2], connection->socket->outcoming_buffer[sizeof(message_header_t) + 3],
        //     connection->socket->outcoming_buffer[sizeof(message_header_t) + 4], connection->socket->outcoming_buffer[sizeof(message_header_t) + 5],
        //     connection->socket->outcoming_buffer[sizeof(message_header_t) + 6], connection->socket->outcoming_buffer[sizeof(message_header_t) + 7]);
    }
    else
    {
        DEBUG_LOG("net_connection_write_data: Message length is 0, no data to write.\n");
    }
    return NET_CONNECTION_STATUS_SUCCESS;
}

net_connection_status_t net_connection_add_to_queue(net_connection_t* connection, message_queue_t* message_queue, message_t* message)
{
    if (!connection || !message || !message_queue)
    {
        DEBUG_LOG("net_connection_add_to_incoming_queue: Null argument(s)\n");
        if (!message)
        {
            message_delete(message);
        }
        return NET_CONNECTION_STATUS_ERROR_NULL_POINTER;
    }

    message_queue_status_t queue_status = message_queue_enqueue(message_queue, message);
    if (queue_status != MESSAGE_QUEUE_STATUS_SUCCESS)
    {
        DEBUG_LOG("net_connection_add_to_message_queue: Error enqueueing message: %d\n", queue_status);
        message_delete(message);
        return NET_CONNECTION_STATUS_ERROR_QUEUE;
    }

    return NET_CONNECTION_STATUS_SUCCESS;
}
