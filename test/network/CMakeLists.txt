add_executable(test_message test_message.c)
target_link_libraries(test_message PRIVATE network utility cunit)

add_test(NAME MessageTests COMMAND test_message)

