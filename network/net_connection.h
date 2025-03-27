#ifndef CONNECTION_H
#define CONNECTION_H

#include <pthread.h>

#include "message_queue.h"
#include "socket_udp.h"

// ********************************* //

typedef enum owner_type
{
    OWNER_SERVER,
    OWNER_CLIENT
} owner_type_t;

// ********************************* //

typedef struct udp_socket udp_socket_t;
typedef struct message_queue message_queue_t;

// ********************************* //

typedef enum net_connection_status
{
    NET_CONNECTION_STATUS_SUCCESS = 0,
    NET_CONNECTION_STATUS_ERROR_NULL_POINTER,
    NET_CONNECTION_STATUS_ERROR_SOCKET,
    NET_CONNECTION_STATUS_ERROR_QUEUE,
    NET_CONNECTION_STATUS_ERROR_THREAD,
    NET_CONNECTION_STATUS_ERROR_MESSAGE_FORMAT
} net_connection_status_t;

// ********************************* //

typedef struct net_connection
{
    owner_type_t owner_type;
    udp_socket_t* socket;
    message_queue_t* incoming_message_queue;
    pthread_t receive_thread;
    int is_running;
} net_connection_t;

// ********************************* //

// Функция потока для приема сообщений
void* net_connection_receive_thread_func(void* arg);

// ********************************* //

// Создает и инициализирует сетевое соединение.
// Возвращает указатель на созданное соединение или NULL в случае ошибки.
net_connection_t* net_connection_create(owner_type_t owner_type, udp_socket_t* socket, message_queue_t* incoming_message_queue);

// Уничтожает сетевое соединение, останавливает поток приема и освобождает выделенные ресурсы.
void net_connection_delete(net_connection_t* connection);

// Запускает процесс приема сообщений в отдельном потоке.
net_connection_status_t net_connection_start_receiving(net_connection_t* connection);

// Останавливает поток приема сообщений.
net_connection_status_t net_connection_stop_receiving(net_connection_t* connection);

// Отправляет сообщение через соединение.
net_connection_status_t net_connection_send(net_connection_t* connection, const message_t* message);

// Принимает входящее сообщение из очереди сообщений (блокирующая операция).
message_t* net_connection_receive(net_connection_t* connection);

// Принимает входящее сообщение из очереди сообщений (неблокирующая операция).
message_t* net_connection_receive_nonblocking(net_connection_t* connection);

// Читает заголовок сообщения из сокета
static net_connection_status_t net_connection_read_header(net_connection_t* connection, message_header_t* header);

// Читает данные сообщения из сокета
static net_connection_status_t net_connection_read_data(net_connection_t* connection, message_t* message);

// Записывает заголовок сообщения в сокет
static net_connection_status_t net_connection_write_header(net_connection_t* connection, const message_header_t* header);

// Записывает данные сообщения в сокет
static net_connection_status_t net_connection_write_data(net_connection_t* connection, const message_t* message);

// Добавляет полученное сообщение в очередь входящих сообщений
static net_connection_status_t net_connection_add_to_incoming_queue(net_connection_t* connection, message_t* message);

// ********************************* //

#endif    // CONNECTION_H