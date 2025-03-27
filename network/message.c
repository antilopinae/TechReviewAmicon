#include "message.h"

#include <stdlib.h>
#include <string.h>

// ********************************* //

message_t* message_create(message_type_t type, uint32_t data_capacity)
{
    if (data_capacity > UINT32_MAX - sizeof(message_header_t))
    {
        return NULL;
    }

    message_t* message = (message_t*)malloc(sizeof(message_header_t) + data_capacity);
    if (message == NULL)
    {
        return NULL;
    }

    message->header.type = type;
    message->header.length = data_capacity;

    return message;
}

// ********************************* //

void message_delete(message_t* message)
{
    if (message != NULL)
    {
        SAFE_FREE(message);
    }
}

// ********************************* //

uint32_t message_size(message_t* message)
{
    if (message == NULL)
    {
        return 0;
    }
    return sizeof(message_header_t) + message->header.length;
}

// ********************************* //

message_status_t message_write(message_t* message, const uint8_t* data, uint32_t length)
{
    if (message == NULL || data == NULL)
    {
        return MESSAGE_STATUS_ERROR_NULL_POINTER;
    }

    if (message->header.length < length)
    {
        return MESSAGE_STATUS_ERROR_WRITE;
    }

    if (length > 0)
    {
        memcpy(message->data, data, length);
    }

    message->header.length = length;

    return MESSAGE_STATUS_SUCCESS;
}

// ********************************* //

message_status_t message_read(message_t* message, uint8_t* data, uint32_t length)
{
    if (message == NULL || data == NULL)
    {
        return MESSAGE_STATUS_ERROR_NULL_POINTER;
    }

    uint32_t data_to_read = (length < message->header.length) ? length : message->header.length;

    if (data_to_read > 0)
    {
        memcpy(data, message->data, data_to_read);
    }
    else
    {
        return MESSAGE_STATUS_ERROR_READ;
    }

    return MESSAGE_STATUS_SUCCESS;
}

// ********************************* //