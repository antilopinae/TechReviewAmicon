link_libraries(network utility cunit)

add_executable(test_message test_message.c)
add_executable(test_message_queue test_message_queue.c)
add_executable(test_socket_udp test_socket_udp.c)
add_executable(test_net_connection test_net_connection.c)

add_test(NAME MessageTest COMMAND test_message)
add_test(NAME MessageQueueTest COMMAND test_message_queue)
add_test(NAME UDPSocketTest COMMAND test_socket_udp)
add_test(NAME NetConnectionTest COMMAND test_net_connection)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    include(Format)
    Format(test_message ${CMAKE_CURRENT_LIST_DIR})
    Format(test_message_queue ${CMAKE_CURRENT_LIST_DIR})
    Format(test_socket_udp ${CMAKE_CURRENT_LIST_DIR})
    Format(test_net_connection ${CMAKE_CURRENT_LIST_DIR})
endif()