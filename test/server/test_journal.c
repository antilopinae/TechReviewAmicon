#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <CUnit/Basic.h>

#include "journal.h"
#include "utility.h"

void test_journal_create_success(void)
{
    journal_t* journal = journal_create(1024);
    CU_ASSERT_PTR_NOT_NULL_FATAL(journal);
    CU_ASSERT_PTR_NOT_NULL(journal->journal_ptr);
    CU_ASSERT_EQUAL(journal->max_size, 1024);
    CU_ASSERT_EQUAL(journal_delete(journal), JOURNAL_STATUS_SUCCESS);
}

void test_journal_write_success(void)
{
    journal_t* journal = journal_create(1024);
    CU_ASSERT_PTR_NOT_NULL_FATAL(journal);
    const char* data = "Test data";
    size_t data_size = strlen(data) + 1;
    CU_ASSERT_EQUAL(journal_write(journal, data, data_size), JOURNAL_STATUS_SUCCESS);
    CU_ASSERT_EQUAL(journal_delete(journal), JOURNAL_STATUS_SUCCESS);
}

void test_journal_write_no_space(void)
{
    journal_t* journal = journal_create(10);
    CU_ASSERT_PTR_NOT_NULL_FATAL(journal);
    const char* data = "Too much data for journal";
    size_t data_size = strlen(data) + 1;
    CU_ASSERT_EQUAL(journal_write(journal, data, data_size), JOURNAL_STATUS_ERROR_NO_SPACE);
    CU_ASSERT_EQUAL(journal_delete(journal), JOURNAL_STATUS_SUCCESS);
}

void test_journal_read_success(void)
{
    journal_t* journal = journal_create(1024);
    CU_ASSERT_PTR_NOT_NULL_FATAL(journal);
    const char* write_data = "Data to read";
    size_t write_data_size = strlen(write_data) + 1;
    CU_ASSERT_EQUAL(journal_write(journal, write_data, write_data_size), JOURNAL_STATUS_SUCCESS);

    char buffer[1024];
    size_t buf_size = 1024;
    CU_ASSERT_EQUAL(journal_read(journal, buffer, &buf_size), JOURNAL_STATUS_SUCCESS);

    CU_ASSERT_EQUAL(buf_size, write_data_size);
    CU_ASSERT_STRING_EQUAL(buffer, write_data);
    CU_ASSERT_EQUAL(journal_delete(journal), JOURNAL_STATUS_SUCCESS);
}

void test_journal_read_buffer_too_small(void)
{
    journal_t* journal = journal_create(1024);
    CU_ASSERT_PTR_NOT_NULL_FATAL(journal);
    const char* write_data = "Large data";
    size_t write_data_size = strlen(write_data) + 1;
    CU_ASSERT_EQUAL(journal_write(journal, write_data, write_data_size), JOURNAL_STATUS_SUCCESS);

    char buffer[5];
    size_t buf_size = sizeof(buffer);
    CU_ASSERT_EQUAL(journal_read(journal, buffer, &buf_size), JOURNAL_STATUS_ERROR_READ);
    CU_ASSERT_EQUAL(journal_delete(journal), JOURNAL_STATUS_SUCCESS);
    CU_ASSERT_EQUAL(buf_size, 0);
}

void test_journal_read_empty_journal(void)
{
    journal_t* journal = journal_create(1024);
    CU_ASSERT_PTR_NOT_NULL_FATAL(journal);
    char buffer[1024];
    size_t buf_size = sizeof(buffer);

    CU_ASSERT_EQUAL(journal_read(journal, buffer, &buf_size), JOURNAL_STATUS_SUCCESS);
    buffer[buf_size] = '\0';
    CU_ASSERT_STRING_EQUAL(buffer, "");
    CU_ASSERT_EQUAL(journal_delete(journal), JOURNAL_STATUS_SUCCESS);
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    if (CUE_SUCCESS != CU_initialize_registry())
    {
        return CU_get_error();
    }

    pSuite = CU_add_suite("JournalTest", NULL, NULL);
    if (NULL == pSuite)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if ((NULL == CU_add_test(pSuite, "create_success", test_journal_create_success)) || (NULL == CU_add_test(pSuite, "write_success", test_journal_write_success))
        || (NULL == CU_add_test(pSuite, "write_no_space", test_journal_write_no_space)) || (NULL == CU_add_test(pSuite, "read_success", test_journal_read_success))
        || (NULL == CU_add_test(pSuite, "read_buffer_too_small", test_journal_read_buffer_too_small))
        || (NULL == CU_add_test(pSuite, "read_empty_journal", test_journal_read_empty_journal)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}