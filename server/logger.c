// logger.c
#include <stdio.h>      // Для fprintf, perror, printf
#include <stdlib.h>     // Для exit, EXIT_SUCCESS, EXIT_FAILURE
#include <unistd.h>     // Для sleep
#include <string.h>     // Для strlen, memcpy
#include <errno.h>      // Для errno
#include <time.h>       // Для time, ctime

#include "shared_memory.h" // Include our shared memory API header

// ********************************* //

// Функция для записи сообщения в разделяемую память
shared_memory_status_t logger_write_message(shared_memory_t *shm, const char *message) {
    if (!shm || !message) {
        fprintf(stderr, "logger_write_message: Null argument(s)\n");
        return SHARED_MEMORY_STATUS_ERROR_NULL_POINTER;
    }

    size_t message_len = strlen(message);
    size_t required_space = message_len + 1; // +1 for null terminator
    size_t current_offset = shm->offset;

    if (current_offset + required_space > shm->size) {
        fprintf(stderr, "logger_write_message: Shared memory overflow, not enough space to write message\n");
        return SHARED_MEMORY_STATUS_ERROR_MEMORY_ALLOC; // Reusing MEMORY_ALLOC status for overflow
    }

    if (pthread_mutex_lock(&shm->memory_mutex) != 0) {
        perror("logger_write_message: pthread_mutex_lock failed");
        return SHARED_MEMORY_STATUS_ERROR_MUTEX_LOCK;
    }

    memcpy((char *)shm->memory_region + current_offset, message, message_len);
    *((char *)shm->memory_region + current_offset + message_len) = '\0'; // Null terminate the string
    shm->offset += required_space; // Update offset for next write

    if (pthread_mutex_unlock(&shm->memory_mutex) != 0) {
        perror("logger_write_message: pthread_mutex_unlock failed");
        return SHARED_MEMORY_STATUS_ERROR_MUTEX_UNLOCK;
    }

    return SHARED_MEMORY_STATUS_SUCCESS;
}

// ********************************* //

int main() {
    shared_memory_t *shm = NULL;
    shared_memory_status_t shm_status;

    // 1. Open the existing shared memory
    shm = shared_memory_open();
    if (!shm) {
        fprintf(stderr, "logger: Failed to open shared memory: %d\n", SHARED_MEMORY_STATUS_ERROR_SHM_OPEN);
        return EXIT_FAILURE;
    }

    printf("Logger process started, writing to shared memory '%s'\n", SHARED_MEMORY_NAME);

    // 2. Write "Hello world" to shared memory every second
    while (1) {
        time_t now = time(NULL);
        char *time_str = ctime(&now); // Get current time string
        if (time_str) {
            // ctime adds a newline at the end, removing it
            size_t len = strlen(time_str);
            if (len > 0 && time_str[len - 1] == '\n') {
                time_str[len - 1] = '\0';
            }
        } else {
            time_str = "Unknown Time";
        }

        char log_message[256]; // Buffer to format log message
        snprintf(log_message, sizeof(log_message), "[%s] Hello world from logger process", time_str);

        shm_status = logger_write_message(shm, log_message);
        if (shm_status != SHARED_MEMORY_STATUS_SUCCESS) {
            fprintf(stderr, "logger: Error writing to shared memory: %d\n", shm_status);
            // In a real logger, you might want to handle errors more gracefully, e.g., retry, log to stderr, etc.
        } else {
            printf("logger: Written to shared memory: '%s'\n", log_message);
        }

        sleep(1); // Wait for 1 second
    }

    // 3. (Unreachable in infinite loop) Close shared memory - for cleanup in case of signal termination.
    // In a real application, handle signals (like SIGINT, SIGTERM) to exit gracefully and close shared memory.
    shm_status = shared_memory_close(shm);
    if (shm_status != SHARED_MEMORY_STATUS_SUCCESS) {
        fprintf(stderr, "logger: Failed to close shared memory: %d\n", shm_status);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// ********************************* //