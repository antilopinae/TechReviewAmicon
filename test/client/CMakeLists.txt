link_libraries(client_lib server_lib utility cunit network)

add_executable(test_client test_client.c)

add_test(NAME ClientTest COMMAND test_client)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    include(Format)
    Format(test_client ${CMAKE_CURRENT_LIST_DIR})
endif()