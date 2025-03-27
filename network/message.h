#ifndef MESSAGE_H
#define MESSAGE_H

#include <netinet/in.h>
#include <stdint.h>

#include "utility.h"

typedef enum message_type
{
    MESSAGE_TYPE_NONE = 0,
    MESSAGE_TYPE_PID = 1,
} message_type_t;

typedef enum message_status
{
    MESSAGE_STATUS_SUCCESS = 0,
    MESSAGE_STATUS_ERROR_NULL_POINTER,
    MESSAGE_STATUS_ERROR_MEMORY_ALLOC,
    MESSAGE_STATUS_ERROR_WRITE,
    MESSAGE_STATUS_ERROR_READ
} message_status_t;

typedef struct message_header
{
    message_type_t type;
    uint32_t length;    // The length of the message data (in bytes), not including the header
    struct sockaddr_in owner_addr;
    uint32_t capacity;
} message_header_t;

typedef struct message
{
    message_header_t header;
    uint8_t* data;
} message_t;

// Creates and initializes a message of the specified type and with the specified data capacity
// Returns a pointer to the created message or NULL in case of a memory allocation error
message_t* message_create(message_type_t type, uint32_t data_capacity);

// Frees up the memory allocated for the message
void message_delete(message_t* message);

// Returns the total size of the message in bytes (header + data)
uint32_t message_size(message_t* message);

// Writes data to the message data buffer
// Returns MESSAGE_STATUS_SUCCESS if successful, otherwise an error code
message_status_t message_write(message_t* message, const uint8_t* data, uint32_t length);

// Reads data from the message data buffer
// Returns MESSAGE_STATUS_SUCCESS if successful, otherwise an error code
message_status_t message_read(message_t* message, uint8_t* data, uint32_t length, uint32_t* read_length);

// Adds the sender address to the message
message_status_t message_add_owner_addr(message_t* message, struct sockaddr_in owner_addr);

#endif    // MESSAGE_H