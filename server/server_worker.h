#ifndef SERVER_WORKER_H
#define SERVER_WORKER_H

#include <pthread.h>    // Для потоков
#include <netinet/in.h> // Для sockaddr_in

#include "socket_udp.h"   // Для UDP сокетов
#include "message_queue.h" // Для очереди сообщений
#include "connection.h"    // Для сетевого соединения (net_connection_t)

// ********************************* //

// Forward declarations
typedef struct net_connection net_connection_t;
typedef struct message_queue message_queue_t;

// ********************************* //

// Статус операций ServerWorker
typedef enum server_worker_status {
    SERVER_WORKER_STATUS_SUCCESS = 0,
    SERVER_WORKER_STATUS_ERROR_NULL_POINTER,
    SERVER_WORKER_STATUS_ERROR_CREATE_CONNECTION,
    SERVER_WORKER_STATUS_ERROR_START_THREADS,
    SERVER_WORKER_STATUS_ERROR_QUEUE_OPERATION,
    SERVER_WORKER_STATUS_ERROR_SOCKET_OPERATION,
    SERVER_WORKER_STATUS_ERROR_THREAD_OPERATION
} server_worker_status_t;

// ********************************* //

// Структура ServerWorker, управляющая сетевым соединением и обработкой сообщений
typedef struct server_worker {
    net_connection_t *connection;         // Сетевое соединение
    message_queue_t *message_queue;       // Очередь входящих сообщений (дублирование, возможно, стоит убрать и использовать из connection)
    pthread_t network_thread;          // Поток для сетевого соединения (приема сообщений)
    pthread_t message_process_thread;    // Поток для обработки и вывода сообщений (в данном случае, печать на экран)
    int is_running;                    // Флаг, указывающий, работает ли worker
} server_worker_t;

// ********************************* //

// Функция потока для обработки и вывода сообщений
void *server_worker_message_process_thread_func(void *arg);

// ********************************* //

// Создает и инициализирует структуру ServerWorker.
// Возвращает указатель на созданную структуру или NULL в случае ошибки.
server_worker_t *server_worker_create(in_addr_t addr, in_port_t port);

// Уничтожает структуру ServerWorker и освобождает выделенные ресурсы.
void server_worker_delete(server_worker_t *worker);

// Запускает рабочие потоки ServerWorker (сетевой и обработки сообщений).
server_worker_status_t server_worker_start(server_worker_t *worker);

// Останавливает рабочие потоки ServerWorker.
server_worker_status_t server_worker_stop(server_worker_t *worker);

// Ожидает входящего соединения (в данном примере для UDP, ожидания соединения как такового нет, но можно интерпретировать как ожидание первого сообщения).
void server_worker_wait_for_connection(server_worker_t *worker); // Функция может быть пустой или реализовывать специфическое поведение, если необходимо

// Отправляет сообщение через соединение.
server_worker_status_t server_worker_send_message(server_worker_t *worker, const message_t *message);

// Обновляет состояние worker, обрабатывая входящие сообщения из очереди.
void server_worker_update(server_worker_t *worker, size_t max_messages); // Убрал STATUS bWait, так как ожидание реализовано в message_queue_dequeue

// Функция обработки сообщения (пример: печать сообщения на экран).
void server_worker_on_message(server_worker_t *worker, message_t *message);

// ********************************* //

#endif // SERVER_WORKER_H