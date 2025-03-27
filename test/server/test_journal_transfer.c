#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <CUnit/Basic.h>

#include "journal.h"
#include "journal_transfer.h"
#include "utility.h"

#define JOURNAL_BUFFER_SIZE 1024
#define JOURNAL_TRANSFER_MES "Hello world from parent process"
#define JOURNAL_FILE_PATH "JOURNAL_TRANSFER_TEST.txt"
#define JOURNAL_TRANSFER_SOCKET_PATH "/tmp/journal"

void test_journal_transfer(void)
{
    pid_t pid = fork();
    CU_ASSERT_NOT_EQUAL_FATAL(pid, -1);

    if (pid == 0)
    {
        FILE* file = fopen(JOURNAL_FILE_PATH, "w");
        CU_ASSERT_PTR_NOT_NULL_FATAL(file);
        fclose(file);

        CU_ASSERT_EQUAL_FATAL(journal_transfer_rcv_and_write_file(JOURNAL_TRANSFER_SOCKET_PATH, JOURNAL_FILE_PATH), JOURNAL_STATUS_SUCCESS);
        exit(EXIT_SUCCESS);
    }
    else
    {
        journal_t* journal = journal_create(JOURNAL_BUFFER_SIZE);
        CU_ASSERT_PTR_NOT_NULL_FATAL(journal);

        const char* message = "Hello world from parent process";

        CU_ASSERT_EQUAL_FATAL(journal_write(journal, message, strlen(message) + 1), JOURNAL_STATUS_SUCCESS);

        DEBUG_LOG("Parent process wrote to journal: '%s'\n", message);

        CU_ASSERT_EQUAL(journal_transfer_run_receiver(JOURNAL_TRANSFER_SOCKET_PATH, journal), 0);

        printf("Triggering journal transfer to file...\n");

        journal_delete(journal);
    }

    wait(NULL);

    FILE* file = fopen(JOURNAL_FILE_PATH, "r");
    CU_ASSERT_PTR_NOT_NULL_FATAL(file);

    char buffer[100];

    CU_ASSERT_PTR_NOT_NULL_FATAL(fgets(buffer, sizeof(buffer), file));
    fclose(file);

    CU_ASSERT_STRING_EQUAL(buffer, JOURNAL_TRANSFER_MES);
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

    if ((NULL == CU_add_test(pSuite, "test_journal_transfer", test_journal_transfer)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}