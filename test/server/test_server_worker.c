#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>

#include "message.h"
#include "net_connection.h"
#include "server_worker.h"
#include "utility.h"

#define TEST_PORT 12350
#define TEST_ADDR "127.0.0.1"
#define TEST_MESSAGE "Test message from client"
#define TEST_MESSAGE_SIZE sizeof(TEST_MESSAGE)

void test_server_worker_create_delete()
{
    server_worker_t* worker = server_worker_create(inet_addr(TEST_ADDR), htons(TEST_PORT));
    CU_ASSERT_PTR_NOT_NULL(worker);
    server_worker_delete(worker);
}

void test_server_worker_start_stop()
{
    server_worker_t* worker = server_worker_create(inet_addr(TEST_ADDR), htons(TEST_PORT));
    CU_ASSERT_PTR_NOT_NULL(worker);

    CU_ASSERT_EQUAL(server_worker_start(worker), SERVER_WORKER_STATUS_SUCCESS);
    CU_ASSERT_EQUAL(worker->is_running, 1);

    CU_ASSERT_EQUAL(server_worker_stop(worker), SERVER_WORKER_STATUS_SUCCESS);
    CU_ASSERT_EQUAL(worker->is_running, 0);

    server_worker_delete(worker);
}

void test_server_worker_wait_for_connection()
{
    server_worker_t* worker = server_worker_create(inet_addr(TEST_ADDR), htons(TEST_PORT));
    CU_ASSERT_PTR_NOT_NULL(worker);

    pthread_t wait_thread;
    pthread_create(&wait_thread, NULL, (void* (*)(void*))server_worker_wait_for_connection, worker);

    // Simulate client sending a message
    udp_socket_t* client_socket = udp_socket_create(inet_addr(TEST_ADDR), htons(TEST_PORT + 1));
    CU_ASSERT_PTR_NOT_NULL(client_socket);
    CU_ASSERT_EQUAL(udp_socket_bind(client_socket), UDP_SOCKET_STATUS_SUCCESS);

    message_queue_t* outgoing_queue_client = message_queue_create(5);
    CU_ASSERT_PTR_NOT_NULL(outgoing_queue_client);
    message_queue_t* incoming_queue_client = message_queue_create(5);
    CU_ASSERT_PTR_NOT_NULL(incoming_queue_client);

    net_connection_t* client_connection = net_connection_create(client_socket, incoming_queue_client, outgoing_queue_client, 1, 1);
    CU_ASSERT_PTR_NOT_NULL(client_connection);
    net_connection_start_sending(client_connection);

    message_t* message_to_send = message_create(MESSAGE_TYPE_PID, TEST_MESSAGE_SIZE);
    CU_ASSERT_PTR_NOT_NULL(message_to_send);
    message_write(message_to_send, (const uint8_t*)TEST_MESSAGE, TEST_MESSAGE_SIZE);
    message_to_send->header.owner_addr = worker->connection->socket->outcoming_addr;

    net_connection_status_t send_status = net_connection_send(client_connection, message_to_send);
    CU_ASSERT_EQUAL(send_status, NET_CONNECTION_STATUS_SUCCESS);

    pthread_join(wait_thread, NULL);

    net_connection_delete(client_connection);

    server_worker_delete(worker);
}

void test_server_worker_send_message()
{
    server_worker_t* worker = server_worker_create(inet_addr(TEST_ADDR), htons(TEST_PORT));
    CU_ASSERT_PTR_NOT_NULL(worker);

    server_worker_status_t start_status = server_worker_start(worker);
    CU_ASSERT_EQUAL(start_status, SERVER_WORKER_STATUS_SUCCESS);

    udp_socket_t* client_socket_recv = udp_socket_create(inet_addr(TEST_ADDR), htons(TEST_PORT + 1));
    CU_ASSERT_PTR_NOT_NULL(client_socket_recv);
    udp_socket_bind(client_socket_recv);

    message_queue_t* incoming_queue_client = message_queue_create(5);
    CU_ASSERT_PTR_NOT_NULL(incoming_queue_client);
    message_queue_t* outgoing_queue_client = message_queue_create(5);
    CU_ASSERT_PTR_NOT_NULL(outgoing_queue_client);

    net_connection_t* client_connection_recv = net_connection_create(client_socket_recv, incoming_queue_client, outgoing_queue_client, 1, 1);
    CU_ASSERT_PTR_NOT_NULL(client_connection_recv);
    net_connection_start_receiving(client_connection_recv);

    struct sockaddr_in client_addr = client_socket_recv->outcoming_addr;

    message_t* message_to_send = message_create(MESSAGE_TYPE_PID, TEST_MESSAGE_SIZE);
    CU_ASSERT_PTR_NOT_NULL(message_to_send);

    message_write(message_to_send, (const uint8_t*)TEST_MESSAGE, TEST_MESSAGE_SIZE);
    message_add_owner_addr(message_to_send, client_addr);

    server_worker_status_t send_msg_status = server_worker_send_message(worker, message_to_send);
    CU_ASSERT_EQUAL(send_msg_status, SERVER_WORKER_STATUS_SUCCESS);

    message_t* received_message = net_connection_receive(client_connection_recv);
    CU_ASSERT_PTR_NOT_NULL(received_message);

    uint8_t received_data[256];
    uint32_t read_length;
    message_status_t read_status = message_read(received_message, received_data, sizeof(received_data), &read_length);
    CU_ASSERT_EQUAL(read_status, MESSAGE_STATUS_SUCCESS);

    CU_ASSERT_STRING_EQUAL((const char*)received_data, TEST_MESSAGE);
    message_delete(received_message);

    net_connection_delete(client_connection_recv);
    server_worker_delete(worker);
}

void test_server_worker_update_and_on_message()
{
    server_worker_t* worker = server_worker_create(inet_addr(TEST_ADDR), htons(TEST_PORT));
    CU_ASSERT_PTR_NOT_NULL(worker);

    server_worker_status_t start_status = server_worker_start(worker);
    CU_ASSERT_EQUAL(start_status, SERVER_WORKER_STATUS_SUCCESS);

    // Create client net_connection to send message to server_worker
    udp_socket_t* client_socket_send = udp_socket_create(inet_addr(TEST_ADDR), htons(TEST_PORT + 1));
    CU_ASSERT_PTR_NOT_NULL(client_socket_send);
    udp_socket_bind(client_socket_send);

    message_queue_t* outgoing_queue_client = message_queue_create(5);
    CU_ASSERT_PTR_NOT_NULL(outgoing_queue_client);
    message_queue_t* incoming_queue_client = message_queue_create(5);
    CU_ASSERT_PTR_NOT_NULL(incoming_queue_client);

    net_connection_t* client_connection_send = net_connection_create(client_socket_send, incoming_queue_client, outgoing_queue_client, 1, 1);
    CU_ASSERT_PTR_NOT_NULL(client_connection_send);
    net_connection_start_sending(client_connection_send);

    message_t* message_to_server = message_create(MESSAGE_TYPE_PID, TEST_MESSAGE_SIZE);
    CU_ASSERT_PTR_NOT_NULL(message_to_server);
    message_write(message_to_server, (const uint8_t*)TEST_MESSAGE, TEST_MESSAGE_SIZE);
    message_to_server->header.owner_addr = worker->connection->socket->outcoming_addr;

    net_connection_status_t send_status = net_connection_send(client_connection_send, message_to_server);
    CU_ASSERT_EQUAL(send_status, NET_CONNECTION_STATUS_SUCCESS);

    server_worker_update(worker, 1);

    net_connection_delete(client_connection_send);
    server_worker_delete(worker);
}

int main()
{
    if (CUE_SUCCESS != CU_initialize_registry())
    {
        return CU_get_error();
    }

    CU_pSuite suite = CU_add_suite("ServerWorkerTest", NULL, NULL);
    if (NULL == suite)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if ((NULL == CU_add_test(suite, "test_server_worker_create_delete", test_server_worker_create_delete))
        || (NULL == CU_add_test(suite, "test_server_worker_start_stop", test_server_worker_start_stop))
        || (NULL == CU_add_test(suite, "test_server_worker_wait_for_connection", test_server_worker_wait_for_connection))
        || (NULL == CU_add_test(suite, "test_server_worker_send_message", test_server_worker_send_message))
        || (NULL == CU_add_test(suite, "test_server_worker_update_and_on_message", test_server_worker_update_and_on_message)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}