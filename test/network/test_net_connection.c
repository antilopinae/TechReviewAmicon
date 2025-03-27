#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>

#include "message.h"
#include "message_queue.h"
#include "net_connection.h"
#include "socket_udp.h"
#include "utility.h"

#define TEST_PORT 12350
#define TEST_ADDR "127.0.0.1"
#define TEST_MESSAGE "Test message from client"
#define TEST_MESSAGE_SIZE sizeof(TEST_MESSAGE)

udp_socket_t* create_test_socket(in_addr_t addr, in_port_t port)
{
    udp_socket_t* sock = udp_socket_create(addr, port);
    if (sock == NULL)
    {
        DEBUG_LOG("udp_socket_create failed");
        return NULL;
    }
    if (udp_socket_bind(sock) != UDP_SOCKET_STATUS_SUCCESS)
    {
        DEBUG_LOG("udp_socket_bind failed");
        udp_socket_delete(sock);
        return NULL;
    }
    return sock;
}

void test_net_connection_create_delete()
{
    udp_socket_t* socket = create_test_socket(inet_addr(TEST_ADDR), htons(TEST_PORT));
    CU_ASSERT_PTR_NOT_NULL(socket);

    message_queue_t* incoming_queue = message_queue_create(5);
    CU_ASSERT_PTR_NOT_NULL(incoming_queue);

    message_queue_t* outgoing_queue = message_queue_create(5);
    CU_ASSERT_PTR_NOT_NULL(outgoing_queue);

    net_connection_t* connection = net_connection_create(socket, incoming_queue, outgoing_queue, 1, 1);
    CU_ASSERT_PTR_NOT_NULL(connection);

    net_connection_delete(connection);
}

void test_net_connection_start_stop_receiving()
{
    udp_socket_t* socket = create_test_socket(inet_addr(TEST_ADDR), htons(TEST_PORT));
    CU_ASSERT_PTR_NOT_NULL(socket);

    message_queue_t* incoming_queue = message_queue_create(5);
    CU_ASSERT_PTR_NOT_NULL(incoming_queue);

    message_queue_t* outgoing_queue = message_queue_create(5);
    CU_ASSERT_PTR_NOT_NULL(outgoing_queue);

    net_connection_t* connection = net_connection_create(socket, incoming_queue, outgoing_queue, 1, 1);
    CU_ASSERT_PTR_NOT_NULL(connection);

    net_connection_status_t status = net_connection_start_receiving(connection);
    CU_ASSERT_EQUAL(status, NET_CONNECTION_STATUS_SUCCESS);
    CU_ASSERT_EQUAL(connection->is_receiving, 1);

    status = net_connection_stop_receiving(connection);
    CU_ASSERT_EQUAL(status, NET_CONNECTION_STATUS_SUCCESS);
    CU_ASSERT_EQUAL(connection->is_receiving, 0);

    net_connection_delete(connection);
}

void test_net_connection_start_stop_sending()
{
    udp_socket_t* socket = create_test_socket(inet_addr(TEST_ADDR), htons(TEST_PORT));
    CU_ASSERT_PTR_NOT_NULL(socket);

    message_queue_t* incoming_queue = message_queue_create(5);
    CU_ASSERT_PTR_NOT_NULL(incoming_queue);

    message_queue_t* outgoing_queue = message_queue_create(5);
    CU_ASSERT_PTR_NOT_NULL(outgoing_queue);

    net_connection_t* connection = net_connection_create(socket, incoming_queue, outgoing_queue, 1, 1);
    CU_ASSERT_PTR_NOT_NULL(connection);

    net_connection_status_t status = net_connection_start_sending(connection);
    CU_ASSERT_EQUAL(status, NET_CONNECTION_STATUS_SUCCESS);
    CU_ASSERT_EQUAL(connection->is_sending, 1);

    status = net_connection_stop_sending(connection);
    CU_ASSERT_EQUAL(status, NET_CONNECTION_STATUS_SUCCESS);
    CU_ASSERT_EQUAL(connection->is_sending, 0);

    net_connection_delete(connection);
}

void test_net_connection_send_receive()
{
    udp_socket_t* socket_send = create_test_socket(inet_addr(TEST_ADDR), htons(TEST_PORT));
    CU_ASSERT_PTR_NOT_NULL(socket_send);

    udp_socket_t* socket_recv = create_test_socket(inet_addr(TEST_ADDR), htons(TEST_PORT + 1));
    CU_ASSERT_PTR_NOT_NULL(socket_recv);

    message_queue_t* incoming_queue_recv = message_queue_create(5);
    CU_ASSERT_PTR_NOT_NULL(incoming_queue_recv);

    message_queue_t* outgoing_queue_recv = message_queue_create(5);
    CU_ASSERT_PTR_NOT_NULL(outgoing_queue_recv);

    message_queue_t* incoming_queue_send = message_queue_create(5);
    CU_ASSERT_PTR_NOT_NULL(incoming_queue_send);

    message_queue_t* outgoing_queue_send = message_queue_create(5);
    CU_ASSERT_PTR_NOT_NULL(outgoing_queue_send);

    net_connection_t* connection_send = net_connection_create(socket_send, incoming_queue_send, outgoing_queue_send, 1, 1);
    CU_ASSERT_PTR_NOT_NULL(connection_send);

    net_connection_t* connection_recv = net_connection_create(socket_recv, incoming_queue_recv, outgoing_queue_recv, 1, 1);
    CU_ASSERT_PTR_NOT_NULL(connection_recv);

    struct sockaddr_in recv_addr = socket_recv->outcoming_addr;

    message_t* message = message_create(MESSAGE_TYPE_PID, 100);
    CU_ASSERT_PTR_NOT_NULL(message);
    message_write(message, (const uint8_t*)TEST_MESSAGE, TEST_MESSAGE_SIZE);

    net_connection_status_t status = net_connection_start_sending(connection_send);
    CU_ASSERT_EQUAL(status, NET_CONNECTION_STATUS_SUCCESS);

    status = net_connection_start_receiving(connection_recv);
    CU_ASSERT_EQUAL(status, NET_CONNECTION_STATUS_SUCCESS);

    message->header.owner_addr = recv_addr;

    status = net_connection_send(connection_send, message);
    CU_ASSERT_EQUAL(status, NET_CONNECTION_STATUS_SUCCESS);

    // Wait a short time
    sleep(1);

    message_t* received_message = net_connection_receive(connection_recv);
    CU_ASSERT_PTR_NOT_NULL(received_message);
    CU_ASSERT_EQUAL(received_message->header.type, MESSAGE_TYPE_PID);

    uint8_t received_data[100];
    uint32_t read_length;
    message_status_t read_status = message_read(received_message, received_data, sizeof(received_data), &read_length);

    CU_ASSERT_EQUAL(read_status, MESSAGE_STATUS_SUCCESS);
    CU_ASSERT_EQUAL(read_length, TEST_MESSAGE_SIZE);
    CU_ASSERT_STRING_EQUAL((const char*)received_data, TEST_MESSAGE);

    net_connection_stop_sending(connection_send);
    net_connection_stop_receiving(connection_recv);

    message_delete(received_message);
    message_delete(message);

    net_connection_delete(connection_send);
    net_connection_delete(connection_recv);
}

int main()
{
    if (CUE_SUCCESS != CU_initialize_registry())
    {
        return CU_get_error();
    }

    CU_pSuite suite = CU_add_suite("NetConnectionTest", NULL, NULL);
    if (NULL == suite)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if ((NULL == CU_add_test(suite, "test_net_connection_create_delete", test_net_connection_create_delete))
        || (NULL == CU_add_test(suite, "test_net_connection_start_stop_receiving", test_net_connection_start_stop_receiving))
        || (NULL == CU_add_test(suite, "test_net_connection_start_stop_sending", test_net_connection_start_stop_sending))
        || (NULL == CU_add_test(suite, "test_net_connection_send_receive", test_net_connection_send_receive)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}