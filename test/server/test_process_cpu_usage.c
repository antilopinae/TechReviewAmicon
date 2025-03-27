#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>

#include "process_cpu_usage.h"
#include "utility.h"

void test_get_process_cpu_usage_valid_pid()
{
    pid_t pid = getpid();
    double cpu_usage = get_process_cpu_usage(pid);

    DEBUG_LOG("CPU usage for PID %d: %.2f%%", pid, cpu_usage);

    CU_ASSERT_NOT_EQUAL(-1.0, cpu_usage);
    CU_ASSERT_TRUE(cpu_usage >= 0.0);
}

void test_get_process_cpu_usage_invalid_pid()
{
    pid_t invalid_pid = 999999999;
    double cpu_usage = get_process_cpu_usage(invalid_pid);

    DEBUG_LOG("CPU usage for invalid PID %d: %.2f%%", invalid_pid, cpu_usage);

    CU_ASSERT_EQUAL(-1.0, cpu_usage);
}

void test_get_process_cpu_usage_zero_elapsed_time()
{
    pid_t pid = getpid();
    double cpu_usage = get_process_cpu_usage(pid);
    CU_ASSERT_NOT_EQUAL(-1.0, cpu_usage);

    for (int i = 0; i < 10; ++i)
    {
        cpu_usage = get_process_cpu_usage(pid);
        CU_ASSERT_NOT_EQUAL(-1.0, cpu_usage);
    }
}

int main(void)
{
    if (CUE_SUCCESS != CU_initialize_registry())
    {
        return CU_get_error();
    }

    CU_pSuite suite = CU_add_suite("ProcessCPUUsageTest", NULL, NULL);
    if (NULL == suite)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if ((NULL == CU_add_test(suite, "test_get_process_cpu_usage_valid_pid", test_get_process_cpu_usage_valid_pid))
        || (NULL == CU_add_test(suite, "test_get_process_cpu_usage_invalid_pid", test_get_process_cpu_usage_invalid_pid))
        || (NULL == CU_add_test(suite, "test_get_process_cpu_usage_zero_elapsed_time", test_get_process_cpu_usage_zero_elapsed_time)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}