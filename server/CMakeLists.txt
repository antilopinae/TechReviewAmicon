set(SERVER_SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB SERVER_SOURCES ${SERVER_SOURCES_DIR}/*.c)
list(FILTER SERVER_SOURCES EXCLUDE REGEX "journal_utility.c|server.c")

add_library(server_lib ${SERVER_SOURCES})
add_executable(server ${SERVER_SOURCES_DIR}/server.c)

add_executable(journal_utility
        ${SERVER_SOURCES_DIR}/journal_utility.c
)

target_link_libraries(server_lib PRIVATE network utility)
target_link_libraries(server PRIVATE server_lib network utility)
target_link_libraries(journal_utility PRIVATE utility server_lib)

target_include_directories(server_lib PUBLIC ${SERVER_SOURCES_DIR})
target_include_directories(journal_utility PUBLIC ${SERVER_SOURCES_DIR})

target_compile_options(server PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(journal_utility PRIVATE -Wall -Wextra -Wpedantic)

target_compile_definitions(server PRIVATE $<$<CONFIG:Debug>:DEBUG>)
target_compile_definitions(server_lib PRIVATE $<$<CONFIG:Debug>:DEBUG>)
target_compile_definitions(journal_utility PRIVATE $<$<CONFIG:Debug>:DEBUG>)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    include(Format)
    Format(server ${SERVER_SOURCES_DIR})
    Format(server_lib ${SERVER_SOURCES_DIR})
    Format(journal_utility ${SERVER_SOURCES_DIR})
endif()