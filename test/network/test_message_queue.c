#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "CUnit/Basic.h"

#include "message.h"
#include "message_queue.h"
#include "utility.h"

message_t* create_test_message(message_type_t type, uint32_t data_capacity, const char* data)
{
    message_t* msg = message_create(type, data_capacity);
    if (msg == NULL)
    {
        return NULL;
    }
    if (data != NULL)
    {
        message_write(msg, (const uint8_t*)data, strlen(data));
    }
    return msg;
}

void destroy_test_message(message_t* msg)
{
    if (msg != NULL)
    {
        message_delete(msg);
    }
}

void test_message_queue_create_success(void)
{
    message_queue_t* queue = message_queue_create(5);
    CU_ASSERT_PTR_NOT_NULL_FATAL(queue);
    CU_ASSERT_EQUAL(queue->capacity, 5);
    CU_ASSERT_EQUAL(queue->head, 0);
    CU_ASSERT_EQUAL(queue->tail, 0);
    message_queue_delete(queue);
}

void test_message_queue_create_zero_capacity(void)
{
    message_queue_t* queue = message_queue_create(0);
    CU_ASSERT_PTR_NULL_FATAL(queue);
}

void test_message_queue_enqueue_success(void)
{
    message_queue_t* queue = message_queue_create(3);
    CU_ASSERT_PTR_NOT_NULL_FATAL(queue);

    message_t* msg1 = create_test_message(MESSAGE_TYPE_PID, 10, "Message 1");
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg1);

    message_queue_status_t status = message_queue_enqueue(queue, msg1);
    CU_ASSERT_EQUAL(status, MESSAGE_QUEUE_STATUS_SUCCESS);
    CU_ASSERT_EQUAL(queue->tail, 1);

    message_queue_delete(queue);
}

void test_message_queue_enqueue_queue_full_overflow(void)
{
    message_queue_t* queue = message_queue_create(4);
    CU_ASSERT_PTR_NOT_NULL_FATAL(queue);

    message_t* msg1 = create_test_message(MESSAGE_TYPE_PID, 10, "Message 1");
    message_t* msg2 = create_test_message(MESSAGE_TYPE_PID, 10, "Message 2");
    message_t* msg3 = create_test_message(MESSAGE_TYPE_PID, 10, "Message 3");
    // Overflow message
    message_t* msg4 = create_test_message(MESSAGE_TYPE_PID, 10, "Message 4");

    CU_ASSERT_EQUAL(message_queue_enqueue(queue, msg1), MESSAGE_QUEUE_STATUS_SUCCESS);
    CU_ASSERT_EQUAL(message_queue_enqueue(queue, msg2), MESSAGE_QUEUE_STATUS_SUCCESS);
    CU_ASSERT_EQUAL(message_queue_enqueue(queue, msg3), MESSAGE_QUEUE_STATUS_SUCCESS);

    CU_ASSERT_EQUAL(queue->tail, 3);
    CU_ASSERT_EQUAL(queue->head, 0);

    message_queue_status_t status = message_queue_enqueue(queue, msg4);
    CU_ASSERT_EQUAL(status, MESSAGE_QUEUE_STATUS_SUCCESS);
    CU_ASSERT_EQUAL(queue->tail, 0);
    CU_ASSERT_EQUAL(queue->head, 2);

    message_t* dequeued_msg;
    dequeued_msg = message_queue_dequeue(queue);
    CU_ASSERT_PTR_NOT_NULL_FATAL(dequeued_msg);

    destroy_test_message(dequeued_msg);

    dequeued_msg = message_queue_dequeue(queue);

    CU_ASSERT_PTR_NOT_NULL_FATAL(dequeued_msg);
    destroy_test_message(dequeued_msg);

    message_queue_delete(queue);
}

void test_message_queue_dequeue_success(void)
{
    message_queue_t* queue = message_queue_create(3);
    CU_ASSERT_PTR_NOT_NULL_FATAL(queue);

    message_t* msg1 = create_test_message(MESSAGE_TYPE_PID, 10, "Message 1");
    message_t* msg2 = create_test_message(MESSAGE_TYPE_PID, 10, "Message 2");
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg1);
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg2);

    CU_ASSERT_EQUAL(message_queue_enqueue(queue, msg1), MESSAGE_QUEUE_STATUS_SUCCESS);
    CU_ASSERT_EQUAL(message_queue_enqueue(queue, msg2), MESSAGE_QUEUE_STATUS_SUCCESS);

    message_t* dequeued_msg1 = message_queue_dequeue(queue);
    CU_ASSERT_PTR_EQUAL(dequeued_msg1, msg1);
    CU_ASSERT_EQUAL(queue->head, 1);

    message_t* dequeued_msg2 = message_queue_dequeue(queue);
    CU_ASSERT_PTR_EQUAL(dequeued_msg2, msg2);
    CU_ASSERT_EQUAL(queue->head, 2);

    message_queue_delete(queue);
    destroy_test_message(dequeued_msg1);
    destroy_test_message(dequeued_msg2);
}

void test_message_queue_dequeue_empty_queue_blocking(void)
{
    message_queue_t* queue = message_queue_create(3);
    CU_ASSERT_PTR_NOT_NULL_FATAL(queue);

    pthread_t dequeue_thread;
    message_t* dequeued_msg_in_thread = NULL;

    void* dequeue_function(void* arg)
    {
        dequeued_msg_in_thread = message_queue_dequeue(queue);
        return NULL;
    }

    CU_ASSERT_EQUAL(pthread_create(&dequeue_thread, NULL, dequeue_function, NULL), 0);

    sleep(1);

    message_t* msg1 = create_test_message(MESSAGE_TYPE_PID, 10, "Message from thread");
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg1);
    CU_ASSERT_EQUAL(message_queue_enqueue(queue, msg1), MESSAGE_QUEUE_STATUS_SUCCESS);

    CU_ASSERT_EQUAL(pthread_join(dequeue_thread, NULL), 0);

    CU_ASSERT_PTR_EQUAL(dequeued_msg_in_thread, msg1);
    destroy_test_message(dequeued_msg_in_thread);

    message_queue_delete(queue);
}

void test_message_queue_dequeue_nonblocking_success(void)
{
    message_queue_t* queue = message_queue_create(3);
    CU_ASSERT_PTR_NOT_NULL_FATAL(queue);

    message_t* msg1 = create_test_message(MESSAGE_TYPE_PID, 10, "Message 1");
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg1);
    CU_ASSERT_EQUAL(message_queue_enqueue(queue, msg1), MESSAGE_QUEUE_STATUS_SUCCESS);

    message_t* dequeued_msg = message_queue_dequeue_nonblocking(queue);
    CU_ASSERT_PTR_EQUAL(dequeued_msg, msg1);
    destroy_test_message(dequeued_msg);
    message_queue_delete(queue);
}

void test_message_queue_dequeue_nonblocking_empty_queue(void)
{
    message_queue_t* queue = message_queue_create(3);
    CU_ASSERT_PTR_NOT_NULL_FATAL(queue);

    message_t* dequeued_msg = message_queue_dequeue_nonblocking(queue);
    CU_ASSERT_PTR_NULL(dequeued_msg);

    message_queue_delete(queue);
}

void test_message_queue_wait_success(void)
{
    message_queue_t* queue = message_queue_create(3);
    CU_ASSERT_PTR_NOT_NULL_FATAL(queue);

    message_t* msg1 = create_test_message(MESSAGE_TYPE_PID, 10, "Message 1");
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg1);
    CU_ASSERT_EQUAL(message_queue_enqueue(queue, msg1), MESSAGE_QUEUE_STATUS_SUCCESS);

    message_t* waited_msg = message_queue_wait(queue);
    CU_ASSERT_PTR_EQUAL(waited_msg, msg1);
    destroy_test_message(waited_msg);
    message_queue_delete(queue);
}

void test_message_queue_wait_empty_queue_blocking(void)
{
    message_queue_t* queue = message_queue_create(3);
    CU_ASSERT_PTR_NOT_NULL_FATAL(queue);

    pthread_t wait_thread;
    message_t* waited_msg_in_thread = NULL;

    void* wait_function(void* arg)
    {
        waited_msg_in_thread = message_queue_wait(queue);
        return NULL;
    }

    CU_ASSERT_EQUAL(pthread_create(&wait_thread, NULL, wait_function, NULL), 0);

    sleep(1);

    message_t* msg1 = create_test_message(MESSAGE_TYPE_PID, 10, "Message from thread");
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg1);
    CU_ASSERT_EQUAL(message_queue_enqueue(queue, msg1), MESSAGE_QUEUE_STATUS_SUCCESS);

    CU_ASSERT_EQUAL(pthread_join(wait_thread, NULL), 0);

    CU_ASSERT_PTR_EQUAL(waited_msg_in_thread, msg1);
    destroy_test_message(waited_msg_in_thread);

    message_queue_delete(queue);
}

int main(void)
{
    if (CUE_SUCCESS != CU_initialize_registry())
    {
        return CU_get_error();
    }

    CU_pSuite pSuite = CU_add_suite("MessageQueueTest", NULL, NULL);
    if (NULL == pSuite)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if ((NULL == CU_add_test(pSuite, "test_message_queue_create_success", test_message_queue_create_success))
        || (NULL == CU_add_test(pSuite, "test_message_queue_create_zero_capacity", test_message_queue_create_zero_capacity))
        || (NULL == CU_add_test(pSuite, "test_message_queue_enqueue_success", test_message_queue_enqueue_success))
        || (NULL == CU_add_test(pSuite, "test_message_queue_enqueue_queue_full_overflow", test_message_queue_enqueue_queue_full_overflow))
        || (NULL == CU_add_test(pSuite, "test_message_queue_dequeue_success", test_message_queue_dequeue_success))
        || (NULL == CU_add_test(pSuite, "test_message_queue_dequeue_empty_queue_blocking", test_message_queue_dequeue_empty_queue_blocking))
        || (NULL == CU_add_test(pSuite, "test_message_queue_dequeue_nonblocking_success", test_message_queue_dequeue_nonblocking_success))
        || (NULL == CU_add_test(pSuite, "test_message_queue_dequeue_nonblocking_empty_queue", test_message_queue_dequeue_nonblocking_empty_queue))
        || (NULL == CU_add_test(pSuite, "test_message_queue_wait_success", test_message_queue_wait_success))
        || (NULL == CU_add_test(pSuite, "test_message_queue_wait_empty_queue_blocking", test_message_queue_wait_empty_queue_blocking)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}