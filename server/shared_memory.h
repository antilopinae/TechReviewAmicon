// shared_memory.h
#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <sys/types.h>
#include <pthread.h>

// ********************************* //

#define SHARED_MEMORY_NAME "/my_shared_memory"
#define SHARED_MEMORY_SIZE (5 * 1024 * 1024)

// ********************************* //

typedef enum shared_memory_status {
    SHARED_MEMORY_STATUS_SUCCESS = 0,
    SHARED_MEMORY_STATUS_ERROR_NULL_POINTER,
    SHARED_MEMORY_STATUS_ERROR_MEMORY_ALLOC,
    SHARED_MEMORY_STATUS_ERROR_SHM_OPEN,
    SHARED_MEMORY_STATUS_ERROR_MMAP,
    SHARED_MEMORY_STATUS_ERROR_MUTEX_INIT,
    SHARED_MEMORY_STATUS_ERROR_MUTEX_LOCK,
    SHARED_MEMORY_STATUS_ERROR_MUTEX_UNLOCK,
    SHARED_MEMORY_STATUS_ERROR_MUTEX_DESTROY,
    SHARED_MEMORY_STATUS_ERROR_MUNMAP,
    SHARED_MEMORY_STATUS_ERROR_SHM_UNLINK
} shared_memory_status_t;

// ********************************* //

typedef struct shared_memory {
    void *memory_region;
    int shm_fd;
    pthread_mutex_t memory_mutex;
    size_t size;
} shared_memory_t;

// ********************************* //

// Type of callback function for read operations
typedef shared_memory_status_t (*shared_memory_read_action_t)(shared_memory_t *shm, void *args);

// ********************************* //

// Creates and initializes shared memory
// Creates a shared memory object and maps it to the address space of the process
// Returns a pointer to the shared_memory_t structure or NULL in case of an error
shared_memory_t *shared_memory_create(size_t size);

// Opens the existing shared memory
// Maps an existing shared memory object to the address space of the process
// Returns a pointer to the shared_memory_t structure or NULL in case of an error
shared_memory_t *shared_memory_open();

// Closes and frees up the resources associated with shared memory
// Cancels memory mapping and closes the file descriptor
shared_memory_status_t shared_memory_close(shared_memory_t *shm);

// Destroys shared memory
// In addition to closing, deletes the shared memory object from the system
shared_memory_status_t shared_memory_destroy(shared_memory_t *shm);

// Reads data from shared memory using a callback action
shared_memory_status_t shared_memory_read(shared_memory_t *shm, shared_memory_read_action_t read_action, void *args);

// Writes data to shared memory
shared_memory_status_t shared_memory_write(shared_memory_t *shm, const void *data, size_t data_size, size_t offset);

// ********************************* //

#endif // SHARED_MEMORY_H