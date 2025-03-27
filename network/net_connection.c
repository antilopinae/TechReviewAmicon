#include "net_connection.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// ********************************* //

void* net_connection_receive_thread_func(void* arg)
{
    net_connection_t* connection = (net_connection_t*)arg;
    if (!connection)
    {
        fprintf(stderr, "net_connection_receive_thread_func: Null connection argument\n");
        return NULL;
    }

    while (connection->is_running)
    {
        message_t* received_message = NULL;
        udp_socket_status_t socket_status;

        // Receive data using UDP socket
        socket_status = udp_socket_receive_message(connection->socket, NULL);    // We don't need callback here, data is in socket buffer
        if (socket_status != UDP_SOCKET_STATUS_SUCCESS)
        {
            if (connection->is_running)
            {    // Only log error if not stopping intentionally
                fprintf(stderr, "net_connection_receive_thread_func: Error receiving UDP message: %d\n", socket_status);
            }
            continue;    // Continue to the next iteration, don't terminate thread immediately on recv error
        }

        // Allocate memory for message header and data (initial capacity, can be adjusted)
        received_message = message_create(MESSAGE_TYPE_NONE, UDP_SOCKET_BUFFER_SIZE);    // Use socket buffer size as initial capacity
        if (!received_message)
        {
            fprintf(stderr, "net_connection_receive_thread_func: Failed to create message\n");
            continue;    // Memory allocation failed, try next loop iteration
        }

        // Read header from socket's received buffer
        net_connection_status_t header_status = net_connection_read_header(connection, &received_message->header);
        if (header_status != NET_CONNECTION_STATUS_SUCCESS)
        {
            fprintf(stderr, "net_connection_receive_thread_func: Error reading message header: %d\n", header_status);
            message_delete(received_message);
            continue;
        }

        // Read data from socket's received buffer
        net_connection_status_t data_status = net_connection_read_data(connection, received_message);
        if (data_status != NET_CONNECTION_STATUS_SUCCESS)
        {
            fprintf(stderr, "net_connection_receive_thread_func: Error reading message data: %d\n", data_status);
            message_delete(received_message);
            continue;
        }

        // Add the received message to the incoming queue
        net_connection_status_t queue_status = net_connection_add_to_incoming_queue(connection, received_message);
        if (queue_status != NET_CONNECTION_STATUS_SUCCESS)
        {
            fprintf(stderr, "net_connection_receive_thread_func: Error adding message to queue: %d\n", queue_status);
            message_delete(received_message);    // Free message if queueing failed
            continue;
        }
        // Message successfully added to queue, no need to free it here, consumer will free it
    }

    return NULL;
}

// ********************************* //

net_connection_t* net_connection_create(owner_type_t owner_type, udp_socket_t* socket, message_queue_t* incoming_message_queue)
{
    if (!socket || !incoming_message_queue)
    {
        fprintf(stderr, "net_connection_create: Null socket or queue argument\n");
        return NULL;
    }

    net_connection_t* connection = malloc(sizeof(net_connection_t));
    if (!connection)
    {
        perror("net_connection_create: malloc failed");
        return NULL;
    }

    connection->owner_type = owner_type;
    connection->socket = socket;                                    // Take ownership of the socket pointer
    connection->incoming_message_queue = incoming_message_queue;    // Take ownership of the queue pointer
    connection->is_running = 0;                                     // Initially not running

    // Start receiving thread immediately upon creation
    net_connection_status_t start_status = net_connection_start_receiving(connection);
    if (start_status != NET_CONNECTION_STATUS_SUCCESS)
    {
        fprintf(stderr, "net_connection_create: Failed to start receiving thread: %d\n", start_status);
        free(connection);
        return NULL;
    }

    return connection;
}

// ********************************* //

void net_connection_delete(net_connection_t* connection)
{
    if (!connection)
    {
        return;    // Safe to call with NULL
    }

    net_connection_stop_receiving(connection);    // Stop the receive thread and wait for it to join

    if (connection->socket)
    {
        udp_socket_delete(connection->socket);    // Delete the UDP socket
        connection->socket = NULL;
    }
    if (connection->incoming_message_queue)
    {
        message_queue_delete(connection->incoming_message_queue);    // Delete the message queue
        connection->incoming_message_queue = NULL;
    }

    free(connection);    // Free the connection structure itself
}

// ********************************* //

// Запускает процесс приема сообщений в отдельном потоке.
net_connection_status_t net_connection_start_receiving(net_connection_t* connection)
{
    if (!connection)
    {
        fprintf(stderr, "net_connection_start_receiving: Null connection argument\n");
        return NET_CONNECTION_STATUS_ERROR_NULL_POINTER;
    }

    if (connection->is_running)
    {
        fprintf(stderr, "net_connection_start_receiving: Receiving thread already running\n");
        return NET_CONNECTION_STATUS_SUCCESS;    // Already running is not an error
    }

    connection->is_running = 1;
    int result = pthread_create(&connection->receive_thread, NULL, net_connection_receive_thread_func, connection);
    if (result != 0)
    {
        perror("net_connection_start_receiving: pthread_create failed");
        connection->is_running = 0;    // Reset running flag on failure
        return NET_CONNECTION_STATUS_ERROR_THREAD;
    }

    return NET_CONNECTION_STATUS_SUCCESS;
}

// ********************************* //

// Останавливает поток приема сообщений.
net_connection_status_t net_connection_stop_receiving(net_connection_t* connection)
{
    if (!connection)
    {
        fprintf(stderr, "net_connection_stop_receiving: Null connection argument\n");
        return NET_CONNECTION_STATUS_ERROR_NULL_POINTER;
    }

    if (!connection->is_running)
    {
        fprintf(stderr, "net_connection_stop_receiving: Receiving thread not running\n");
        return NET_CONNECTION_STATUS_SUCCESS;    // Not running is not an error
    }

    connection->is_running = 0;    // Signal thread to stop

    if (pthread_join(connection->receive_thread, NULL) != 0)
    {
        perror("net_connection_stop_receiving: pthread_join failed");
        return NET_CONNECTION_STATUS_ERROR_THREAD;
    }

    return NET_CONNECTION_STATUS_SUCCESS;
}

// ********************************* //

// Отправляет сообщение через соединение.
net_connection_status_t net_connection_send(net_connection_t* connection, const message_t* message)
{
    if (!connection || !message || !connection->socket)
    {
        fprintf(stderr, "net_connection_send: Null argument(s)\n");
        return NET_CONNECTION_STATUS_ERROR_NULL_POINTER;
    }

    net_connection_status_t header_status = net_connection_write_header(connection, &message->header);
    if (header_status != NET_CONNECTION_STATUS_SUCCESS)
    {
        fprintf(stderr, "net_connection_send: Error writing header: %d\n", header_status);
        return header_status;
    }

    net_connection_status_t data_status = net_connection_write_data(connection, message);
    if (data_status != NET_CONNECTION_STATUS_SUCCESS)
    {
        fprintf(stderr, "net_connection_send: Error writing data: %d\n", data_status);
        return data_status;
    }

    udp_socket_status_t send_status = udp_socket_send_to(connection->socket);    // Assumes sending back to last client
    if (send_status != UDP_SOCKET_STATUS_SUCCESS)
    {
        fprintf(stderr, "net_connection_send: Error sending via UDP socket: %d\n", send_status);
        return NET_CONNECTION_STATUS_ERROR_SOCKET;
    }

    return NET_CONNECTION_STATUS_SUCCESS;
}

// ********************************* //

// Принимает входящее сообщение из очереди сообщений (блокирующая операция).
message_t* net_connection_receive(net_connection_t* connection)
{
    if (!connection || !connection->incoming_message_queue)
    {
        fprintf(stderr, "net_connection_receive: Null argument(s)\n");
        return NULL;
    }
    return message_queue_dequeue(connection->incoming_message_queue);
}

// ********************************* //

// Принимает входящее сообщение из очереди сообщений (неблокирующая операция).
message_t* net_connection_receive_nonblocking(net_connection_t* connection)
{
    if (!connection || !connection->incoming_message_queue)
    {
        fprintf(stderr, "net_connection_receive_nonblocking: Null argument(s)\n");
        return NULL;
    }
    return message_queue_dequeue_nonblocking(connection->incoming_message_queue);
}

// ********************************* //

// Читает заголовок сообщения из сокета (низкоуровневая функция, для внутреннего использования).
static net_connection_status_t net_connection_read_header(net_connection_t* connection, message_header_t* header)
{
    if (!connection || !header || !connection->socket)
    {
        fprintf(stderr, "net_connection_read_header: Null argument(s)\n");
        return NET_CONNECTION_STATUS_ERROR_NULL_POINTER;
    }

    // Assuming header is at the beginning of the socket's client buffer
    memcpy(header, connection->socket->incoming_buffer, sizeof(message_header_t));    // Directly copy header

    // Basic validation (optional, but good practice) - check if message type is within valid range, etc.
    // ... (add validation logic here if needed) ...

    return NET_CONNECTION_STATUS_SUCCESS;
}

// ********************************* //

// Читает данные сообщения из сокета (низкоуровневая функция, для внутреннего использования).
static net_connection_status_t net_connection_read_data(net_connection_t* connection, message_t* message)
{
    if (!connection || !message || !connection->socket)
    {
        fprintf(stderr, "net_connection_read_data: Null argument(s)\n");
        return NET_CONNECTION_STATUS_ERROR_NULL_POINTER;
    }

    if (message->header.length > 0)
    {
        if (message->header.length > UDP_SOCKET_BUFFER_SIZE - sizeof(message_header_t))
        {
            fprintf(stderr, "net_connection_read_data: Message length exceeds buffer capacity\n");
            return NET_CONNECTION_STATUS_ERROR_MESSAGE_FORMAT;
        }
        memcpy(message->data, connection->socket->incoming_buffer + sizeof(message_header_t), message->header.length);    // Copy data after header
    }
    return NET_CONNECTION_STATUS_SUCCESS;
}

// ********************************* //

// Записывает заголовок сообщения в сокет (низкоуровневая функция, для внутреннего использования).
static net_connection_status_t net_connection_write_header(net_connection_t* connection, const message_header_t* header)
{
    if (!connection || !header || !connection->socket)
    {
        fprintf(stderr, "net_connection_write_header: Null argument(s)\n");
        return NET_CONNECTION_STATUS_ERROR_NULL_POINTER;
    }

    // Assuming we are using server buffer to send
    memcpy(connection->socket->outcoming_buffer, header, sizeof(message_header_t));    // Copy header to server buffer

    return NET_CONNECTION_STATUS_SUCCESS;
}

// ********************************* //

// Записывает данные сообщения в сокет (низкоуровневая функция, для внутреннего использования).
static net_connection_status_t net_connection_write_data(net_connection_t* connection, const message_t* message)
{
    if (!connection || !message || !connection->socket)
    {
        fprintf(stderr, "net_connection_write_data: Null argument(s)\n");
        return NET_CONNECTION_STATUS_ERROR_NULL_POINTER;
    }

    if (message->header.length > 0)
    {
        if (message->header.length > UDP_SOCKET_BUFFER_SIZE - sizeof(message_header_t))
        {
            fprintf(stderr, "net_connection_write_data: Message length exceeds buffer capacity\n");
            return NET_CONNECTION_STATUS_ERROR_MESSAGE_FORMAT;
        }
        memcpy(connection->socket->outcoming_buffer + sizeof(message_header_t), message->data, message->header.length);    // Copy data after header
    }
    return NET_CONNECTION_STATUS_SUCCESS;
}

// ********************************* //

// Добавляет полученное сообщение в очередь входящих сообщений (внутренняя функция, вызывается из потока приема).
static net_connection_status_t net_connection_add_to_incoming_queue(net_connection_t* connection, message_t* message)
{
    if (!connection || !message || !connection->incoming_message_queue)
    {
        fprintf(stderr, "net_connection_add_to_incoming_queue: Null argument(s)\n");
        message_delete(message);    // Free message if queue is invalid or NULL, prevent leak
        return NET_CONNECTION_STATUS_ERROR_NULL_POINTER;
    }

    message_queue_status_t queue_status = message_queue_enqueue(connection->incoming_message_queue, message);
    if (queue_status != MESSAGE_QUEUE_STATUS_SUCCESS)
    {
        fprintf(stderr, "net_connection_add_to_incoming_queue: Error enqueueing message: %d\n", queue_status);
        message_delete(message);    // Free message if enqueue failed, prevent leak
        return NET_CONNECTION_STATUS_ERROR_QUEUE;
    }

    return NET_CONNECTION_STATUS_SUCCESS;
}

// ********************************* //