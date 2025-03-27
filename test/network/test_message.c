#include <CUnit/Basic.h>

#include "message.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void test_message_create() {
    message_t *msg = message_create(MESSAGE_TYPE_PID, 10);
    CU_ASSERT_PTR_NOT_NULL(msg);
    if (msg) {
        CU_ASSERT_EQUAL(msg->header.type, MESSAGE_TYPE_PID);
        CU_ASSERT_EQUAL(msg->header.length, 0);
        message_delete(msg);
    }
}

void test_message_write_read() {
    uint8_t data[] = {1, 2, 3, 4, 5};
    uint8_t buffer[5] = {0};

    message_t *msg = message_create(MESSAGE_TYPE_PID, 5);
    CU_ASSERT_PTR_NOT_NULL(msg);

    if (msg) {
        CU_ASSERT_EQUAL(message_write(msg, data, 5), MESSAGE_STATUS_SUCCESS);
        CU_ASSERT_EQUAL(message_read(msg, buffer, 5), MESSAGE_STATUS_SUCCESS);
        CU_ASSERT_EQUAL(memcmp(data, buffer, 5), 0);
        message_delete(msg);
    }
}

void test_message_size() {
    message_t *msg = message_create(MESSAGE_TYPE_PID, 10);
    CU_ASSERT_PTR_NOT_NULL(msg);

    if (msg) {
        CU_ASSERT_EQUAL(message_size(msg), sizeof(message_header_t) + 10);
        message_delete(msg);
    }
}

int main() {
    CU_initialize_registry();
    CU_pSuite suite = CU_add_suite("Message Tests", NULL, NULL);

    CU_add_test(suite, "Test message_create", test_message_create);
    CU_add_test(suite, "Test message_write_read", test_message_write_read);
    CU_add_test(suite, "Test message_size", test_message_size);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return 0;
}
