#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

#include "shared_memory.h" // Include our shared memory API header

typedef struct shared_data {
    int counter;
} shared_data_t;

int main() {
    shared_memory_t *shm = NULL; // Use our shared_memory_t structure
    shared_data_t *shared_mem_ptr;  // Pointer to the shared data region
    pid_t worker_pid1, worker_pid2;
    int status;
    shared_memory_status_t shm_status;

    size_t shm_size = sizeof(shared_data_t);

    // 1. Create shared memory segment using our API
    shm = shared_memory_create(shm_size);
    if (!shm) {
        fprintf(stderr, "Failed to create shared memory: %d\n", SHARED_MEMORY_STATUS_ERROR_SHM_OPEN);
        return EXIT_FAILURE;
    }

    // Get pointer to the shared memory region
    shared_mem_ptr = (shared_data_t*)shm->memory_region;
    if (!shared_mem_ptr) {
        fprintf(stderr, "Shared memory region is NULL after create\n");
        shared_memory_destroy(shm);
        return EXIT_FAILURE;
    }

    // Initialize shared data
    shared_mem_ptr->counter = 0;
    printf("Parent process: Shared memory allocated and initialized. Initial counter: %d\n", shared_mem_ptr->counter);

    // 2. Fork the first worker process
    worker_pid1 = fork();
    if (worker_pid1 == -1) {
        perror("fork failed for worker 1");
        shared_memory_destroy(shm);
        return EXIT_FAILURE;
    }

    if (worker_pid1 == 0) { // Child process 1 (worker 1)
        shared_memory_t *child_shm = NULL;
        shared_data_t *child_shared_mem_ptr = NULL;

        // Open existing shared memory in child process using our API
        child_shm = shared_memory_open();
        if (!child_shm) {
            fprintf(stderr, "Worker 1 failed to open shared memory: %d\n", SHARED_MEMORY_STATUS_ERROR_SHM_OPEN);
            exit(EXIT_FAILURE);
        }
        child_shared_mem_ptr = (shared_data_t*)child_shm->memory_region;
        if (!child_shared_mem_ptr) {
            fprintf(stderr, "Worker 1 shared memory region is NULL after open\n");
            shared_memory_close(child_shm);
            exit(EXIT_FAILURE);
        }

        printf("Worker process 1 (PID: %d): Attached to shared memory. Counter before increment: %d\n", getpid(), child_shared_mem_ptr->counter);
        child_shared_mem_ptr->counter++; // Increment the counter in shared memory
        printf("Worker process 1 (PID: %d): Counter after increment: %d\n", getpid(), child_shared_mem_ptr->counter);

        shared_memory_close(child_shm); // Close shared memory in child
        exit(EXIT_SUCCESS); // Child process exits successfully
    }

    // 3. Fork the second worker process
    worker_pid2 = fork();
    if (worker_pid2 == -1) {
        perror("fork failed for worker 2");
        shared_memory_destroy(shm);
        return EXIT_FAILURE;
    }

    if (worker_pid2 == 0) { // Child process 2 (worker 2)
        shared_memory_t *child_shm = NULL;
        shared_data_t *child_shared_mem_ptr = NULL;

        // Open existing shared memory in child process using our API
        child_shm = shared_memory_open();
        if (!child_shm) {
            fprintf(stderr, "Worker 2 failed to open shared memory: %d\n", SHARED_MEMORY_STATUS_ERROR_SHM_OPEN);
            exit(EXIT_FAILURE);
        }
        child_shared_mem_ptr = (shared_data_t*)child_shm->memory_region;
        if (!child_shared_mem_ptr) {
            fprintf(stderr, "Worker 2 shared memory region is NULL after open\n");
            shared_memory_close(child_shm);
            exit(EXIT_FAILURE);
        }

        printf("Worker process 2 (PID: %d): Attached to shared memory. Counter before increment: %d\n", getpid(), child_shared_mem_ptr->counter);
        child_shared_mem_ptr->counter++; // Increment the counter in shared memory
        printf("Worker process 2 (PID: %d): Counter after increment: %d\n", getpid(), child_shared_mem_ptr->counter);

        shared_memory_close(child_shm); // Close shared memory in child
        exit(EXIT_SUCCESS); // Child process exits successfully
    }

    // 4. Parent process waits for both child processes to finish
    printf("Parent process (PID: %d): Waiting for worker processes to complete...\n", getpid());

    if (waitpid(worker_pid1, &status, 0) == -1) {
        perror("waitpid failed for worker 1");
    } else {
        if (WIFEXITED(status)) {
            printf("Parent process: Worker 1 (PID: %d) exited with status %d\n", worker_pid1, WEXITSTATUS(status));
        } else {
            printf("Parent process: Worker 1 (PID: %d) terminated abnormally\n", worker_pid1);
        }
    }

    if (waitpid(worker_pid2, &status, 0) == -1) {
        perror("waitpid failed for worker 2");
    } else {
        if (WIFEXITED(status)) {
            printf("Parent process: Worker 2 (PID: %d) exited with status %d\n", worker_pid2, WEXITSTATUS(status));
        } else {
            printf("Parent process: Worker 2 (PID: %d) terminated abnormally\n", worker_pid2);
        }
    }

    // 5. Parent process reads the final counter value from shared memory
    printf("Parent process: Final counter value in shared memory: %d\n", shared_mem_ptr->counter);

    // 6. Destroy shared memory segment using our API
    shm_status = shared_memory_destroy(shm);
    if (shm_status != SHARED_MEMORY_STATUS_SUCCESS) {
        fprintf(stderr, "Failed to destroy shared memory: %d\n", shm_status);
        return EXIT_FAILURE;
    }
    printf("Parent process: Shared memory segment removed.\n");

    return EXIT_SUCCESS;
}