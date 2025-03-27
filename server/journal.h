#ifndef JOURNAL_H
#define JOURNAL_H

#include <pthread.h>

typedef enum journal_status
{
    JOURNAL_STATUS_SUCCESS = 0,
    JOURNAL_STATUS_ERROR_MUNMAP,
    JOURNAL_STATUS_ERROR_MMAP,
    JOURNAL_STATUS_ERROR_MALLOC,
    JOURNAL_STATUS_ERROR_MUTEX_INIT,
    JOURNAL_STATUS_ERROR_MUTEX_LOCK,
    JOURNAL_STATUS_ERROR_MUTEX_UNLOCK,
    JOURNAL_STATUS_ERROR_WRITE,
    JOURNAL_STATUS_ERROR_READ,
    JOURNAL_STATUS_ERROR_NO_SPACE,
    JOURNAL_STATUS_ERROR_PARAMS_NULL

} journal_status_t;

typedef struct journal
{
    pthread_mutex_t mutex;
    size_t max_size;
    void* journal_ptr;
} journal_t;

// Create journal ANONYMOUS SHARED
journal_t* journal_create(size_t size);

// Delete journal and close if needed
journal_status_t journal_delete(journal_t* journal);

// Write to the end of journal
journal_status_t journal_write(journal_t* journal, const void* data, size_t data_size);

// Copy all journal to buffer, return buffer_size as amount of cur size of journal
journal_status_t journal_read(journal_t* journal, void* buffer, size_t* buffer_size);

#endif    // JOURNAL_H