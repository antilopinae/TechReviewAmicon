#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "config.h"
#include "journal.h"
#include "journal_transfer.h"
#include "process_cpu_usage.h"
#include "server_worker.h"

#include "safe_process.h"
#include "utility.h"

#define SERVER_RESPONSE_CPU_NOT_FOUND "not found"
#define SERVER_RESPONSE_CPU_INVALID "invalid"

typedef struct server_state
{
    int num_workers;
    int base_server_port;
    journal_t* journal;
    const char* server_addr;
    const char* unix_socket_path;
} server_state_t;

static journal_t* journal;

void message_set_data(message_t* message, const char* data)
{
    if (message->header.capacity >= strlen(data) + 1)
    {
        message_write(message, (const uint8_t*)data, strlen(data) + 1);
        return;
    }
    else
    {
        uint8_t* new_data = realloc(message->data, strlen(data) + 1);
        if (!new_data)
        {
            DEBUG_LOG("realloc failed\n");
            return;
        }

        message->data = new_data;
        strcpy(message->data, data);
    }

    message->header.capacity = strlen(data) + 1;
    message->header.length = strlen(data) + 1;
}

void worker_on_message(message_t* message)
{
    char buffer[256];
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);

    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

    if (message->header.type == MESSAGE_TYPE_NONE)
    {
        message->header.type = MESSAGE_TYPE_PID;

        if (snprintf(buffer, sizeof(buffer), "%s: invalid request\n", time_str) < 0)
        {
            perror("Failed to scnprintf cpu invalid");
            return;
        }
        journal_write(journal, buffer, strlen(buffer) + 1);
        message_set_data(message, SERVER_RESPONSE_CPU_INVALID);
    }
    else if (message->header.type == MESSAGE_TYPE_PID)
    {
        pid_t pid = *((int*)message->data);
        printf("New message: type:%d, pid: %d\n", message->header.type, pid);

        double proc_cpu_usage = get_process_cpu_usage(pid);

        if (proc_cpu_usage < 0)
        {
            if (snprintf(buffer, sizeof(buffer), "%s: %d not found\n", time_str, pid) < 0)
            {
                perror("Failed to scnprintf process cpu usage");
                return;
            }

            journal_write(journal, buffer, strlen(buffer) + 1);
            message_set_data(message, SERVER_RESPONSE_CPU_NOT_FOUND);
        }
        else
        {
            if (snprintf(buffer, sizeof(buffer), "%s%s%d%s%f\n", time_str, ": ", pid, " %", proc_cpu_usage) < 0)
            {
                perror("Failed to scnprintf process cpu usage");
                return;
            }
            printf("Write: %s", buffer);
            journal_write(journal, buffer, strlen(buffer) + 1);

            if (snprintf(buffer, sizeof(buffer), "%f", proc_cpu_usage) < 0)
            {
                perror("Failed to scnprintf process cpu usage");
                return;
            }

            message_set_data(message, buffer);
        }
    }
}

void worker_process_job(void* args, safe_process_t sp)
{
    server_state_t* server_state = (server_state_t*)args;

    printf("Worker process started (PID %d), port %d\n", getpid(), server_state->base_server_port);

    server_worker_t* worker = server_worker_create(inet_addr(server_state->server_addr), server_state->base_server_port);
    if (!worker)
    {
        DEBUG_LOG("server_worker_create failed in create\n");
        exit(EXIT_FAILURE);
    }

    server_worker_set_on_message(worker, worker_on_message);

    if (server_worker_start(worker) != SERVER_WORKER_STATUS_SUCCESS)
    {
        DEBUG_LOG("server_worker_start failed\n");
        safe_process_delete_this(sp, worker);
    }

    while (true)
    {
        safe_process_check_status(sp, worker);
        server_worker_update(worker, 100);
        sleep(1);
    }
}

void worker_process_clear(void* args)
{
    server_worker_t* worker = (server_worker_t*)args;
    server_worker_delete(worker);
}

void* sp_check_status_loop(void* arg)
{
    safe_process_t* sp = (safe_process_t*)arg;
    while (1)
    {
        safe_process_check_status(*sp, NULL);
        sleep(1);
    }
}

void* journal_receiver_job(void* data)
{
    server_state_t* server = (server_state_t*)data;

    printf("Journal transfer receiver started\n");

    while (journal_transfer_run_receiver(server->unix_socket_path, server->journal) == 0)
        ;

    DEBUG_LOG("journal_transfer_run_receiver failed in journal_receiver\n");
}

// "Usage: %s <server_addr> <count_ports> <base_port> <max_journal_size> <unix_socket>\n"
int main(int argc, char* argv[])
{
    ssize_t journal_size = SERVER_MAX_JOURNAL_SIZE;
    journal = journal_create(journal_size);
    if (!journal)
    {
        DEBUG_LOG("Journal create failed. Exiting.\n");
        return EXIT_FAILURE;
    }

    // Test write
    journal_write(journal, "Hello from server journal\n", strlen("Hello from server journal\n"));

    server_state_t server_state = {
        .num_workers = SERVER_NUM_WORKERS, .base_server_port = SERVER_BASE_PORT, .server_addr = SERVER_ADDR, .unix_socket_path = SERVER_UNIX_SOCKET_PATH, .journal = journal};

    for (int i = 0; i < SERVER_NUM_WORKERS; i++)
    {
        server_state.base_server_port = SERVER_BASE_PORT + i;
        safe_process_t server_worker = safe_process_create(worker_process_job, &server_state, worker_process_clear);
    }

    pthread_t journal_thread;
    if (pthread_create(&journal_thread, NULL, journal_receiver_job, &server_state) != 0)
    {
        perror("Error create thread");
        exit(EXIT_FAILURE);
    }

    wait(NULL);

    journal_delete(journal);

    return EXIT_SUCCESS;
}
