#include "message_queue.h"

#include <stdlib.h>
#include <string.h>

message_queue_t* message_queue_create(int capacity)
{
    if (capacity <= 0)
    {
        DEBUG_LOG("message_queue_create: Capacity must be positive\n");
        return NULL;
    }

    message_queue_t* queue = malloc(sizeof(message_queue_t));
    if (!queue)
    {
        DEBUG_LOG("message_queue_create: malloc failed");
        return NULL;
    }

    queue->buffer = malloc(sizeof(message_t*) * capacity);
    if (!queue->buffer)
    {
        DEBUG_LOG("message_queue_create: malloc buffer failed");
        goto queue_cleanup;
    }

    if (pthread_mutex_init(&queue->mutex, NULL) != 0)
    {
        DEBUG_LOG("message_queue_create: mutex init failed");
        goto queue_buffer_cleanup;
    }

    if (pthread_cond_init(&queue->not_empty_cond, NULL) != 0)
    {
        DEBUG_LOG("message_queue_create: condition variable init failed");
        goto queue_mutex_cleanup;
    }

    queue->capacity = capacity;
    queue->head = 0;
    queue->tail = 0;

    return queue;

queue_mutex_cleanup:
    pthread_mutex_destroy(&queue->mutex);

queue_buffer_cleanup:
    SAFE_FREE(queue->buffer);

queue_cleanup:
    SAFE_FREE(queue);

    return NULL;
}

void message_queue_delete(message_queue_t* queue)
{
    if (!queue)
    {
        return;
    }

    if (pthread_mutex_lock(&queue->mutex) != 0)
    {
        DEBUG_LOG("message_queue_delete: mutex lock failed during deletion, memory might leak");
    }

    // Iterate through the queue and delete all messages
    int i = queue->head;
    while (i != queue->tail)
    {
        message_t* tmp = queue->buffer[i];
        if (tmp)
        {
            message_delete(tmp);
        }
        i = (i + 1) % queue->capacity;
    }

    queue->head = 0;
    queue->tail = 0;

    if (pthread_mutex_unlock(&queue->mutex) != 0)
    {
        DEBUG_LOG("message_queue_delete: mutex unlock failed during deletion");
    }

    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->not_empty_cond);

    SAFE_FREE(queue->buffer);
    SAFE_FREE(queue);
}

message_queue_status_t message_queue_enqueue(message_queue_t* queue, message_t* message)
{
    if (!queue || !message)
    {
        DEBUG_LOG("message_queue_enqueue: Null pointer argument\n");
        return MESSAGE_QUEUE_STATUS_ERROR_NULL_POINTER;
    }

    if (pthread_mutex_lock(&queue->mutex) != 0)
    {
        DEBUG_LOG("message_queue_enqueue: mutex lock failed");
        return MESSAGE_QUEUE_STATUS_ERROR_MUTEX;
    }

    // Checking if the queue is full
    int next_tail = (queue->tail + 1) % queue->capacity;
    if (next_tail == queue->head)
    {
        // The queue is full - we clear half of the queue
        DEBUG_LOG("Message queue overflow detected. Clearing half of the queue.\n");
        while (queue->head != (next_tail + queue->capacity / 2) % queue->capacity)
        {
            message_t* tmp = queue->buffer[queue->head];
            SAFE_FREE(tmp);
            queue->head = (queue->head + 1) % queue->capacity;
        }
    }

    queue->buffer[queue->tail] = message;
    queue->tail = next_tail;

    if (pthread_cond_signal(&queue->not_empty_cond) != 0)
    {
        DEBUG_LOG("message_queue_enqueue: condition signal failed");
        pthread_mutex_unlock(&queue->mutex);
        return MESSAGE_QUEUE_STATUS_ERROR_CONDVAR;
    }

    if (pthread_mutex_unlock(&queue->mutex) != 0)
    {
        DEBUG_LOG("message_queue_enqueue: mutex unlock failed");
        return MESSAGE_QUEUE_STATUS_ERROR_MUTEX;
    }

    return MESSAGE_QUEUE_STATUS_SUCCESS;
}

message_t* message_queue_dequeue(message_queue_t* queue)
{
    if (!queue)
    {
        DEBUG_LOG("message_queue_dequeue: Null pointer argument\n");
        return NULL;
    }

    if (pthread_mutex_lock(&queue->mutex) != 0)
    {
        DEBUG_LOG("message_queue_dequeue: mutex lock failed");
        return NULL;
    }

    while (queue->head == queue->tail)
    {
        if (pthread_cond_wait(&queue->not_empty_cond, &queue->mutex) != 0)
        {
            DEBUG_LOG("message_queue_dequeue: condition wait failed");
            pthread_mutex_unlock(&queue->mutex);
            return NULL;
        }
        // Wake up when the queue is not empty
    }

    message_t* message = queue->buffer[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;

    if (pthread_mutex_unlock(&queue->mutex) != 0)
    {
        DEBUG_LOG("message_queue_dequeue: mutex unlock failed");
        return NULL;
    }

    return message;
}

message_t* message_queue_dequeue_nonblocking(message_queue_t* queue)
{
    if (!queue)
    {
        DEBUG_LOG("message_queue_dequeue_nonblocking: Null pointer argument\n");
        return NULL;
    }

    if (pthread_mutex_lock(&queue->mutex) != 0)
    {
        DEBUG_LOG("message_queue_dequeue_nonblocking: mutex lock failed");
        return NULL;
    }

    if (queue->head == queue->tail)
    {
        if (pthread_mutex_unlock(&queue->mutex) != 0)
        {
            DEBUG_LOG("message_queue_dequeue_nonblocking: mutex unlock failed");
            return NULL;
        }
        return NULL;
    }

    message_t* message = queue->buffer[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;

    if (pthread_mutex_unlock(&queue->mutex) != 0)
    {
        DEBUG_LOG("message_queue_dequeue_nonblocking: mutex unlock failed");
        return NULL;
    }

    return message;
}

message_t* message_queue_wait(message_queue_t* queue)
{
    // Just calling dequeue, since it already implements waiting
    return message_queue_dequeue(queue);
}
