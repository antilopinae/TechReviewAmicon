set(SERVER_SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB SERVER_SOURCES ${SERVER_SOURCES_DIR}/*.c)

add_executable(server ${SERVER_SOURCES_DIR}/server.c ${SERVER_SOURCES})

add_executable(journal_utility
        ${SERVER_SOURCES_DIR}/journal_utility.c
        ${SERVER_SOURCES_DIR}/shared_memory.c
)

target_link_libraries(server PRIVATE network utility)
target_link_libraries(journal_utility PRIVATE utility)

target_compile_options(server PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(journal_utility PRIVATE -Wall -Wextra -Wpedantic)

target_compile_definitions(server PRIVATE $<$<CONFIG:Debug>:DEBUG>)
target_compile_definitions(journal_utility PRIVATE $<$<CONFIG:Debug>:DEBUG>)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    include(Format)
    Format(server ${SERVER_SOURCES_DIR})
    Format(journal_utility ${SERVER_SOURCES_DIR})
endif()