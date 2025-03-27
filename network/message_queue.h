#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include <pthread.h>

#include "message.h"
#include "utility.h"

typedef enum message_queue_status
{
    MESSAGE_QUEUE_STATUS_SUCCESS = 0,
    MESSAGE_QUEUE_STATUS_ERROR_NULL_POINTER,
    MESSAGE_QUEUE_STATUS_ERROR_MEMORY_ALLOC,
    MESSAGE_QUEUE_STATUS_ERROR_MUTEX,
    MESSAGE_QUEUE_STATUS_ERROR_CONDVAR,
    MESSAGE_QUEUE_STATUS_ERROR_QUEUE_FULL
} message_queue_status_t;

typedef struct message_queue
{
    message_t** buffer;
    int capacity;
    int head;
    int tail;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty_cond;
} message_queue_t;

// Creates and initializes a thread-safe message queue
// Returns a pointer to the created queue or NULL in case of an error
message_queue_t* message_queue_create(int capacity);

// Destroys the message queue and frees up the allocated resources and messages
void message_queue_delete(message_queue_t* queue);

// Adds the message to the end of the queue
// In case of overflow, clears half of the queue
// Returns the operation status
message_queue_status_t message_queue_enqueue(message_queue_t* queue, message_t* message);

// Retrieves and returns a message from the beginning of the queue
// If the queue is empty, it blocks the calling thread until the message appears
// Returns a pointer to the extracted message or NULL in case of an error
message_t* message_queue_dequeue(message_queue_t* queue);

// Retrieves and returns a message from the front of the queue, if there is one
// If the queue is empty, returns NULL immediately
message_t* message_queue_dequeue_nonblocking(message_queue_t* queue);

// The function of waiting for a message to appear in the queue
// Just calls message_queue_dequeue, but it may be useful
message_t* message_queue_wait(message_queue_t* queue);

#endif    // MESSAGE_QUEUE_H