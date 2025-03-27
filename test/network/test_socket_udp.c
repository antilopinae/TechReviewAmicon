#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "CUnit/Basic.h"

#include "socket_udp.h"
#include "utility.h"

#define TEST_PORT 12350
#define TEST_ADDR "127.0.0.1"
#define TEST_MESSAGE "Test message from client"
#define TEST_MESSAGE_SIZE sizeof(TEST_MESSAGE)

static volatile int message_received = 0;
static char received_message[UDP_SOCKET_BUFFER_SIZE];

void test_recv_callback(struct sockaddr_in client_addr, char* client_message, ssize_t message_len)
{
    message_received = 1;
    strncpy(received_message, client_message, message_len);
    received_message[message_len] = '\0';
}

void* send_message_thread(void* arg)
{
    udp_socket_t* client_sock = (udp_socket_t*)arg;

    memcpy(client_sock->outcoming_buffer, TEST_MESSAGE, TEST_MESSAGE_SIZE);
    CU_ASSERT_EQUAL(udp_socket_send(client_sock, TEST_MESSAGE_SIZE), UDP_SOCKET_STATUS_SUCCESS);

    udp_socket_delete(client_sock);

    return NULL;
}

void test_send_recv_message(void)
{
    udp_socket_t* server_sock = udp_socket_create(inet_addr(TEST_ADDR), htons(TEST_PORT));
    CU_ASSERT_PTR_NOT_NULL(server_sock);
    CU_ASSERT_EQUAL(udp_socket_bind(server_sock), UDP_SOCKET_STATUS_SUCCESS);

    message_received = 0;
    memset(received_message, 0, sizeof(received_message));

    udp_socket_t* client_sock = udp_socket_create(inet_addr(TEST_ADDR), 0);
    CU_ASSERT_PTR_NOT_NULL(client_sock);

    CU_ASSERT_EQUAL(udp_socket_bind(client_sock), UDP_SOCKET_STATUS_SUCCESS);

    client_sock->incoming_addr = server_sock->outcoming_addr;

    pthread_t sender_thread;
    pthread_create(&sender_thread, NULL, send_message_thread, client_sock);

    udp_socket_receive_message(server_sock, test_recv_callback);

    pthread_join(sender_thread, NULL);

    CU_ASSERT_TRUE(message_received);
    CU_ASSERT_STRING_EQUAL(received_message, TEST_MESSAGE);

    udp_socket_delete(server_sock);
}

void test_recv_timeout(void)
{
    udp_socket_t* my_socket = udp_socket_create(inet_addr(TEST_ADDR), htons(TEST_PORT));
    CU_ASSERT_PTR_NOT_NULL(my_socket);

    CU_ASSERT_EQUAL(udp_socket_set_receive_timeout(my_socket, 1), UDP_SOCKET_STATUS_SUCCESS);
    CU_ASSERT_EQUAL(udp_socket_bind(my_socket), UDP_SOCKET_STATUS_SUCCESS);

    CU_ASSERT_EQUAL(udp_socket_receive_message(my_socket, NULL), UDP_SOCKET_STATUS_ERROR_RECEIVE);

    udp_socket_delete(my_socket);
}

int main(void)
{
    if (CUE_SUCCESS != CU_initialize_registry())
    {
        return CU_get_error();
    }

    CU_pSuite pSuite = CU_add_suite("UDPSocketTest", NULL, NULL);
    if (!pSuite)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test_send_recv_message", test_send_recv_message) || NULL == CU_add_test(pSuite, "test_recv_timeout", test_recv_timeout))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}