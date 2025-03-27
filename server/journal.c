#include "journal.h"

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "utility.h"

journal_t* journal_create(size_t size)
{
    if (size <= sizeof(size_t))
    {
        DEBUG_LOG("journal_create failed: size is zero");
        return NULL;
    }

    journal_t* journal = (journal_t*)malloc(sizeof(journal_t));
    if (!journal)
    {
        DEBUG_LOG("journal_create failed: malloc error: %s", strerror(errno));
        return NULL;
    }

    journal->max_size = size;
    journal->journal_ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (journal->journal_ptr == MAP_FAILED)
    {
        DEBUG_LOG("journal_create failed: mmap error: %s", strerror(errno));
        goto journal_create_map_failed;
    }

    size_t cur_size = sizeof(size_t);
    memcpy(journal->journal_ptr, &cur_size, sizeof(size_t));

    pthread_mutexattr_t attr;
    if (pthread_mutexattr_init(&attr) != 0)
    {
        DEBUG_LOG("journal_create failed: pthread_mutexattr_init error: %s", strerror(errno));
        goto journal_create_mutex_failed;
    }

    if (pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) != 0)
    {
        DEBUG_LOG("journal_create failed: pthread_mutexattr_setpshared error: %s", strerror(errno));
        goto journal_create_attr_failed;
    }

    if (pthread_mutex_init(&journal->mutex, &attr) != 0)
    {
        DEBUG_LOG("journal_create failed: pthread_mutex_init error: %s", strerror(errno));
        goto journal_create_attr_failed;
    }

    pthread_mutexattr_destroy(&attr);

    return journal;

journal_create_attr_failed:
    pthread_mutexattr_destroy(&attr);

journal_create_mutex_failed:
    munmap(journal->journal_ptr, journal->max_size);

journal_create_map_failed:
    SAFE_FREE(journal);

    return NULL;
}

journal_status_t journal_delete(journal_t* journal)
{
    if (!journal)
    {
        DEBUG_LOG("journal_delete: journal is NULL");
        return JOURNAL_STATUS_ERROR_PARAMS_NULL;
    }

    if (munmap(journal->journal_ptr, journal->max_size) == -1)
    {
        DEBUG_LOG("journal_delete: Error unlinking journal: %s", strerror(errno));
        return JOURNAL_STATUS_ERROR_MUNMAP;
    }

    if (pthread_mutex_destroy(&journal->mutex) != 0)
    {
        DEBUG_LOG("journal_delete: Error destroying mutex: %s", strerror(errno));
    }

    SAFE_FREE(journal);

    return JOURNAL_STATUS_SUCCESS;
}

journal_status_t journal_write(journal_t* journal, const void* data, size_t data_size)
{
    if (!journal || !data)
    {
        DEBUG_LOG("journal_write: journal or data is NULL\n");
        return JOURNAL_STATUS_ERROR_PARAMS_NULL;
    }

    if (data_size == 0)
    {
        DEBUG_LOG("journal_write: data_size is zero\n");
        return JOURNAL_STATUS_SUCCESS;
    }

    if (pthread_mutex_lock(&journal->mutex) != 0)
    {
        DEBUG_LOG("journal_write: pthread_mutex_lock error: %s\n", strerror(errno));
        return JOURNAL_STATUS_ERROR_MUTEX_LOCK;
    }

    size_t cur_size = 0;
    memcpy(&cur_size, journal->journal_ptr, sizeof(size_t));

    if (cur_size + data_size > journal->max_size)
    {
        DEBUG_LOG("journal_write: not enough space in journal. Available: %zu, requested: %zu\n", journal->max_size - cur_size, data_size);
        pthread_mutex_unlock(&journal->mutex);
        return JOURNAL_STATUS_ERROR_NO_SPACE;
    }

    memcpy((char*)journal->journal_ptr + cur_size, data, data_size);
    cur_size += data_size;
    memcpy(journal->journal_ptr, &cur_size, sizeof(cur_size));

    if (pthread_mutex_unlock(&journal->mutex) != 0)
    {
        DEBUG_LOG("journal_write: pthread_mutex_unlock error: %s\n", strerror(errno));
        return JOURNAL_STATUS_ERROR_MUTEX_UNLOCK;
    }

    return JOURNAL_STATUS_SUCCESS;
}

journal_status_t journal_read(journal_t* journal, void* buffer, size_t* buffer_size)
{
    if (!journal || !buffer || !buffer_size)
    {
        DEBUG_LOG("journal_read: journal or buffer or buffer_size is NULL\n");
        return JOURNAL_STATUS_ERROR_PARAMS_NULL;
    }

    if (pthread_mutex_lock(&journal->mutex) != 0)
    {
        DEBUG_LOG("journal_read: pthread_mutex_lock error: %s\n", strerror(errno));
        *buffer_size = 0;
        return JOURNAL_STATUS_ERROR_MUTEX_LOCK;
    }

    size_t cur_size = 0;
    memcpy(&cur_size, journal->journal_ptr, sizeof(size_t));

    if (*buffer_size < cur_size)
    {
        DEBUG_LOG("journal_read: buffer too small. Buffer size: %zu, journal data size: %zu\n", *buffer_size, cur_size);
        *buffer_size = 0;
        pthread_mutex_unlock(&journal->mutex);
        return JOURNAL_STATUS_ERROR_READ;
    }

    memcpy(buffer, (const char*)journal->journal_ptr + sizeof(size_t), cur_size - sizeof(size_t));

    if (pthread_mutex_unlock(&journal->mutex) != 0)
    {
        DEBUG_LOG("journal_read: pthread_mutex_unlock error: %s\n", strerror(errno));
        *buffer_size = 0;
        return JOURNAL_STATUS_ERROR_MUTEX_UNLOCK;
    }

    *buffer_size = cur_size - sizeof(size_t);

    return JOURNAL_STATUS_SUCCESS;
}
