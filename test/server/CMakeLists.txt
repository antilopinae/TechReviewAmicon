link_libraries(server_lib utility cunit network)

add_executable(test_server_worker test_server_worker.c)
add_executable(test_process_cpu_usage test_process_cpu_usage.c)
add_executable(test_journal test_journal.c)
add_executable(test_journal_transfer test_journal_transfer.c)

add_test(NAME ServerWorkerTest COMMAND test_server_worker)
add_test(NAME ProcessCPUUsageTest COMMAND test_process_cpu_usage.c)
add_test(NAME JournalTest COMMAND test_journal.c)
add_test(NAME JournalTransferTest COMMAND test_journal_transfer.c)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    include(Format)
    Format(test_server_worker ${CMAKE_CURRENT_LIST_DIR})
    Format(test_process_cpu_usage ${CMAKE_CURRENT_LIST_DIR})
    Format(test_journal ${CMAKE_CURRENT_LIST_DIR})
    Format(test_journal_transfer ${CMAKE_CURRENT_LIST_DIR})
endif()