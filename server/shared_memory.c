#include "shared_memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

// ********************************* //

shared_memory_t *shared_memory_create(size_t size) {
    if (size == 0 || size > SHARED_MEMORY_SIZE) {
        fprintf(stderr, "shared_memory_create: Invalid size requested: %zu, max allowed: %d\n", size, SHARED_MEMORY_SIZE);
        return NULL;
    }

    shared_memory_t *shm = malloc(sizeof(shared_memory_t));
    if (!shm) {
        perror("shared_memory_create: malloc failed");
        return NULL;
    }

    shm->shm_fd = shm_open(SHARED_MEMORY_NAME, O_RDWR | O_CREAT | O_EXCL, 0666); // O_EXCL to ensure creation only if not exists
    if (shm->shm_fd == -1) {
        perror("shared_memory_create: shm_open failed");
        free(shm);
        return NULL;
    }

    // Setting the size of the shared memory
    if (ftruncate(shm->shm_fd, size) == -1) {
        perror("shared_memory_create: ftruncate failed");
        close(shm->shm_fd);
        shm_unlink(SHARED_MEMORY_NAME); // Clean up shared memory object on error
        free(shm);
        return NULL;
    }

    // Mapping shared memory to the address space of the process
    shm->memory_region = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm->shm_fd, 0);
    if (shm->memory_region == MAP_FAILED) {
        perror("shared_memory_create: mmap failed");
        close(shm->shm_fd);
        shm_unlink(SHARED_MEMORY_NAME); // Clean up shared memory object on error
        free(shm);
        return NULL;
    }

    if (pthread_mutex_init(&shm->memory_mutex, NULL) != 0) {
        perror("shared_memory_create: pthread_mutex_init failed");
        munmap(shm->memory_region, size);
        close(shm->shm_fd);
        shm_unlink(SHARED_MEMORY_NAME); // Clean up shared memory object on error
        free(shm);
        return NULL;
    }

    shm->size = size;

    return shm;
}

// ********************************* //

shared_memory_t *shared_memory_open() {
    shared_memory_t *shm = malloc(sizeof(shared_memory_t));
    if (!shm) {
        perror("shared_memory_open: malloc failed");
        return NULL;
    }

    // No O_CREAT, assume it exists
    shm->shm_fd = shm_open(SHARED_MEMORY_NAME, O_RDWR, 0666);
    if (shm->shm_fd == -1) {
        perror("shared_memory_open: shm_open failed");
        free(shm);
        return NULL;
    }

    // You can do this via fstat, but for simplicity we use a macro, assuming that the size is known.
    size_t size = SHARED_MEMORY_SIZE;

    shm->memory_region = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm->shm_fd, 0);
    if (shm->memory_region == MAP_FAILED) {
        perror("shared_memory_open: mmap failed");
        close(shm->shm_fd);
        free(shm);
        return NULL;
    }

    if (pthread_mutex_init(&shm->memory_mutex, NULL) != 0) {
        perror("shared_memory_open: pthread_mutex_init failed");
        munmap(shm->memory_region, size);
        close(shm->shm_fd);
        free(shm);
        return NULL;
    }

    shm->size = size;

    return shm;
}

// ********************************* //

shared_memory_status_t shared_memory_close(shared_memory_t *shm) {
    if (!shm) {
        fprintf(stderr, "shared_memory_close: Null shared memory pointer\n");
        return SHARED_MEMORY_STATUS_ERROR_NULL_POINTER;
    }

    // Store size before potentially invalidating shm pointer
    size_t size = shm->size;

    if (pthread_mutex_destroy(&shm->memory_mutex) != 0) {
        perror("shared_memory_close: pthread_mutex_destroy failed");
        return SHARED_MEMORY_STATUS_ERROR_MUTEX_DESTROY;
    }

    if (munmap(shm->memory_region, size) == -1) {
        perror("shared_memory_close: munmap failed");
        return SHARED_MEMORY_STATUS_ERROR_MUNMAP;
    }

    if (close(shm->shm_fd) == -1) {
        perror("shared_memory_close: close failed");
        return SHARED_MEMORY_STATUS_ERROR_SHM_OPEN;
    }

    free(shm);

    return SHARED_MEMORY_STATUS_SUCCESS;
}

// ********************************* //

shared_memory_status_t shared_memory_destroy(shared_memory_t *shm) {
    shared_memory_status_t close_status = shared_memory_close(shm);
    if (close_status != SHARED_MEMORY_STATUS_SUCCESS && close_status != SHARED_MEMORY_STATUS_ERROR_NULL_POINTER) {
        fprintf(stderr, "shared_memory_destroy: shared_memory_close failed, but continuing with shm_unlink: %d\n", close_status);
        // Continue even if close failed, try to unlink
    }

    if (shm_unlink(SHARED_MEMORY_NAME) == -1) {
        perror("shared_memory_destroy: shm_unlink failed");
        return SHARED_MEMORY_STATUS_ERROR_SHM_UNLINK;
    }

    return SHARED_MEMORY_STATUS_SUCCESS;
}

// ********************************* //

shared_memory_status_t shared_memory_read(shared_memory_t *shm, shared_memory_read_action_t read_action, void *args) {
    if (!shm || !read_action) {
        fprintf(stderr, "shared_memory_read: Null argument(s)\n");
        return SHARED_MEMORY_STATUS_ERROR_NULL_POINTER;
    }

    if (pthread_mutex_lock(&shm->memory_mutex) != 0) {
        perror("shared_memory_read: pthread_mutex_lock failed");
        return SHARED_MEMORY_STATUS_ERROR_MUTEX_LOCK;
    }

    shared_memory_status_t status = read_action(shm, args);

    if (pthread_mutex_unlock(&shm->memory_mutex) != 0) {
        perror("shared_memory_read: pthread_mutex_unlock failed");
        return SHARED_MEMORY_STATUS_ERROR_MUTEX_UNLOCK;
    }

    return status;
}

// ********************************* //

shared_memory_status_t shared_memory_write(shared_memory_t *shm, const void *data, size_t data_size, size_t offset) {
    if (!shm || !data) {
        fprintf(stderr, "shared_memory_write: Null argument(s)\n");
        return SHARED_MEMORY_STATUS_ERROR_NULL_POINTER;
    }
    if (offset + data_size > shm->size) {
        fprintf(stderr, "shared_memory_write: Write exceeds shared memory bounds\n");
        return SHARED_MEMORY_STATUS_ERROR_MEMORY_ALLOC;
    }

    if (pthread_mutex_lock(&shm->memory_mutex) != 0) {
        perror("shared_memory_write: pthread_mutex_lock failed");
        return SHARED_MEMORY_STATUS_ERROR_MUTEX_LOCK;
    }

    memcpy((char *)shm->memory_region + offset, data, data_size);

    if (pthread_mutex_unlock(&shm->memory_mutex) != 0) {
        perror("shared_memory_write: pthread_mutex_unlock failed");
        return SHARED_MEMORY_STATUS_ERROR_MUTEX_UNLOCK;
    }

    return SHARED_MEMORY_STATUS_SUCCESS;
}

// ********************************* //