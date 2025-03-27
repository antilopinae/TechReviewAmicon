#include "server_worker.h"

#include <stdio.h>      // Для fprintf, perror, printf
#include <stdlib.h>     // Для malloc, free
#include <string.h>     // Для strerror
#include <errno.h>      // Для errno
#include <arpa/inet.h>  // Для inet_ntoa, ntohs

// ********************************* //

// Функция потока для обработки и вывода сообщений
void *server_worker_message_process_thread_func(void *arg) {
    server_worker_t *worker = (server_worker_t *)arg;
    if (!worker) {
        fprintf(stderr, "server_worker_message_process_thread_func: Null worker argument\n");
        return NULL;
    }

    while (worker->is_running) {
        message_t *message = net_connection_receive(worker->connection); // Блокирующее получение сообщения
        if (message) {
            server_worker_on_message(worker, message); // Обработка сообщения (печать на экран)
            message_delete(message); // Освобождаем память сообщения после обработки
        }
        // В случае NULL сообщения (ошибка приема или остановка worker), цикл продолжается, пока worker->is_running
    }

    return NULL;
}

// ********************************* //

// Создает и инициализирует структуру ServerWorker.
server_worker_t *server_worker_create(in_addr_t addr, in_port_t port) {
    server_worker_t *worker = malloc(sizeof(server_worker_t));
    if (!worker) {
        perror("server_worker_create: malloc failed");
        return NULL;
    }

    worker->connection = NULL;
    worker->message_queue = NULL;
    worker->network_thread = 0;
    worker->message_process_thread = 0;
    worker->is_running = 0;

    udp_socket_t *udp_socket = udp_socket_create(addr, htons(port)); // htons для port
    if (!udp_socket) {
        fprintf(stderr, "server_worker_create: Failed to create UDP socket\n");
        free(worker);
        return NULL;
    }

    udp_socket_status_t bind_status = udp_socket_bind(udp_socket);
    if (bind_status != UDP_SOCKET_STATUS_SUCCESS) {
        fprintf(stderr, "server_worker_create: UDP socket bind failed: %d\n", bind_status);
        udp_socket_delete(udp_socket);
        free(worker);
        return NULL;
    }

    message_queue_t *msg_queue = message_queue_create(1024); // Пример размера очереди
    if (!msg_queue) {
        fprintf(stderr, "server_worker_create: Failed to create message queue\n");
        udp_socket_delete(udp_socket);
        free(worker);
        return NULL;
    }

    net_connection_t *conn = net_connection_create(OWNER_SERVER, udp_socket, msg_queue); // Создаем соединение, worker берет владение сокетом и очередью
    if (!conn) {
        fprintf(stderr, "server_worker_create: Failed to create net connection\n");
        message_queue_delete(msg_queue);
        udp_socket_delete(udp_socket);
        free(worker);
        return NULL;
    }

    worker->connection = conn;
    worker->message_queue = msg_queue; // Хотя очередь уже внутри connection, сохраняем указатель (может быть избыточно)

    return worker;
}

// ********************************* //

// Уничтожает структуру ServerWorker и освобождает выделенные ресурсы.
void server_worker_delete(server_worker_t *worker) {
    if (!worker) {
        return; // Safe to call with NULL
    }

    server_worker_stop(worker); // Сначала останавливаем потоки

    if (worker->connection) {
        net_connection_delete(worker->connection); // Удаляем сетевое соединение, что также удалит socket и queue
        worker->connection = NULL;
        worker->message_queue = NULL; // Указатель на очередь уже не валиден после net_connection_delete
    }

    free(worker); // Освобождаем саму структуру worker
}

// ********************************* //

// Запускает рабочие потоки ServerWorker (сетевой и обработки сообщений).
server_worker_status_t server_worker_start(server_worker_t *worker) {
    if (!worker) {
        fprintf(stderr, "server_worker_start: Null worker argument\n");
        return SERVER_WORKER_STATUS_ERROR_NULL_POINTER;
    }

    if (worker->is_running) {
        fprintf(stderr, "server_worker_start: Worker already running\n");
        return SERVER_WORKER_STATUS_SUCCESS; // Уже запущен, не ошибка
    }

    worker->is_running = 1;

    // Запуск потока обработки сообщений
    int msg_thread_result = pthread_create(&worker->message_process_thread, NULL, server_worker_message_process_thread_func, worker);
    if (msg_thread_result != 0) {
        perror("server_worker_start: pthread_create (message_process_thread) failed");
        worker->is_running = 0; // Сброс флага запуска при ошибке
        return SERVER_WORKER_STATUS_ERROR_THREAD_OPERATION;
    }
    // Сетевой поток запускается автоматически при создании net_connection в server_worker_create

    return SERVER_WORKER_STATUS_SUCCESS;
}

// ********************************* //

// Останавливает рабочие потоки ServerWorker.
server_worker_status_t server_worker_stop(server_worker_t *worker) {
    if (!worker) {
        fprintf(stderr, "server_worker_stop: Null worker argument\n");
        return SERVER_WORKER_STATUS_ERROR_NULL_POINTER;
    }

    if (!worker->is_running) {
        fprintf(stderr, "server_worker_stop: Worker not running\n");
        return SERVER_WORKER_STATUS_SUCCESS; // Уже остановлен, не ошибка
    }

    worker->is_running = 0; // Сигнализируем потокам остановиться
    net_connection_stop_receiving(worker->connection); // Останавливаем сетевой поток явно через connection

    // Ожидаем завершения потока обработки сообщений
    if (pthread_join(worker->message_process_thread, NULL) != 0) {
        perror("server_worker_stop: pthread_join (message_process_thread) failed");
        return SERVER_WORKER_STATUS_ERROR_THREAD_OPERATION;
    }
    worker->message_process_thread = 0; // Сброс идентификатора потока

    return SERVER_WORKER_STATUS_SUCCESS;
}

// ********************************* //

// Ожидает входящего соединения (для UDP - ожидания первого сообщения).
void server_worker_wait_for_connection(server_worker_t *worker) {
    // Для UDP, функция может быть пустой, так как нет "соединения" как такового.
    // Или можно реализовать ожидание первого сообщения, если это требуется по логике.
    // В текущей реализации, оставляем пустой.
    if (!worker) {
        fprintf(stderr, "server_worker_wait_for_connection: Null worker argument\n");
        return;
    }
    // Можно добавить логику ожидания первого сообщения из очереди, если нужно
}

// ********************************* //

// Отправляет сообщение через соединение.
server_worker_status_t server_worker_send_message(server_worker_t *worker, const message_t *message) {
    if (!worker || !message || !worker->connection) {
        fprintf(stderr, "server_worker_send_message: Null argument(s)\n");
        return SERVER_WORKER_STATUS_ERROR_NULL_POINTER;
    }
    return net_connection_send(worker->connection, message);
}

// ********************************* //

// Обновляет состояние worker, обрабатывая входящие сообщения из очереди.
void server_worker_update(server_worker_t *worker, size_t max_messages) {
    if (!worker) {
        fprintf(stderr, "server_worker_update: Null worker argument\n");
        return;
    }

    size_t message_count = 0;
    while (message_count < max_messages) {
        message_t *message = net_connection_receive_nonblocking(worker->connection); // Неблокирующее получение
        if (message) {
            server_worker_on_message(worker, message); // Обработка сообщения
            message_delete(message); // Освобождение памяти сообщения
            message_count++;
        } else {
            break; // Нет больше сообщений в очереди или очередь пуста
        }
    }
}

// ********************************* //

// Функция обработки сообщения (пример: печать сообщения на экран).
void server_worker_on_message(server_worker_t *worker, message_t *message) {
    if (!worker || !message) {
        fprintf(stderr, "server_worker_on_message: Null argument(s)\n");
        return;
    }

    struct sockaddr_in *client_addr = &worker->connection->socket->client_addr; // Получаем адрес клиента из сокета
    if (client_addr) {
        printf("Received message from IP: %s and port: %i\n", inet_ntoa(client_addr->sin_addr),
               ntohs(client_addr->sin_port));
    }
    // Предполагаем, что message->data - это нуль-терминированная строка, или обрабатываем длину message->header.length
    printf("Message from client: %.*s\n", message->header.length, message->data);
}

// ********************************* //