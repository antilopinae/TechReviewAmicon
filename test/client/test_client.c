#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>

#include "client.h"
#include "server_worker.h"
#include "utility.h"

#define TEST_PORT 12360
#define TEST_ADDR "127.0.0.1"
#define TEST_PID 12345

void test_client_create_delete(void)
{
    int arr[1] = {TEST_PORT};
    client_context_t* context = client_create_context(CLIENT_MODE_NORMAL, inet_addr(TEST_ADDR), 1, arr);
    CU_ASSERT_PTR_NOT_NULL(context);
    client_delete_context(context);
}

void test_client_start_stop(void)
{
    int arr[1] = {TEST_PORT};
    client_context_t* context = client_create_context(CLIENT_MODE_NORMAL, inet_addr(TEST_ADDR), 1, arr);
    CU_ASSERT_PTR_NOT_NULL(context);

    client_status_t start_status = client_start(context);
    CU_ASSERT_EQUAL(start_status, CLIENT_STATUS_SUCCESS);
    CU_ASSERT_EQUAL(context->is_running, 1);

    client_status_t stop_status = client_stop(context);
    CU_ASSERT_EQUAL(stop_status, CLIENT_STATUS_SUCCESS);
    CU_ASSERT_EQUAL(context->is_running, 0);

    client_delete_context(context);
}

void on_message(message_t* message)
{
    CU_ASSERT_EQUAL_FATAL(message->header.type, MESSAGE_TYPE_PID);
    DEBUG_LOG("Received message\n");
}

void test_client_send_pid_message_normal_mode(void)
{
    server_worker_t* server_worker = server_worker_create(inet_addr(TEST_ADDR), TEST_PORT);
    CU_ASSERT_PTR_NOT_NULL_FATAL(server_worker);
    server_worker_start(server_worker);

    int arr[1] = {TEST_PORT};
    client_context_t* context = client_create_context(CLIENT_MODE_NORMAL, inet_addr(TEST_ADDR), 1, arr);

    CU_ASSERT_PTR_NOT_NULL(context);
    client_status_t start_status = client_start(context);
    CU_ASSERT_EQUAL(start_status, CLIENT_STATUS_SUCCESS);

    client_status_t send_status = client_send_pid_message(context, TEST_PID);
    CU_ASSERT_EQUAL_FATAL(send_status, CLIENT_STATUS_SUCCESS);

    server_worker_wait_for_connection(server_worker);

    server_worker_set_on_message(server_worker, on_message);
    server_worker_update(server_worker, 1);

    client_delete_context(context);
    server_worker_delete(server_worker);
}

int main(void)
{
    if (CUE_SUCCESS != CU_initialize_registry())
    {
        return CU_get_error();
    }

    CU_pSuite suite = CU_add_suite("ClientTest", NULL, NULL);
    if (NULL == suite)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if ((NULL == CU_add_test(suite, "test_client_create_delete", test_client_create_delete)) || (NULL == CU_add_test(suite, "test_client_start_stop", test_client_start_stop))
        || (NULL == CU_add_test(suite, "test_client_send_pid_message_normal_mode", test_client_send_pid_message_normal_mode)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}