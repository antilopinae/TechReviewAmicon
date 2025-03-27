set(CLIENT_SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB CLIENT_SOURCES ${CLIENT_SOURCES_DIR}/*.c)
list(FILTER SERVER_SOURCES EXCLUDE REGEX "client.c")

add_library(client_lib STATIC ${CLIENT_SOURCES})

add_executable(client ${CLIENT_SOURCES_DIR}/boot_client.c)

target_link_libraries(client PRIVATE client_lib)
target_link_libraries(client_lib PRIVATE network utility)

target_compile_options(client_lib PRIVATE -Wall -Wextra -Wpedantic)
target_compile_definitions(client_lib PRIVATE $<$<CONFIG:Debug>:DEBUG>)

target_include_directories(client_lib PUBLIC ${CLIENT_SOURCES_DIR})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    include(Format)
    Format(client ${CLIENT_SOURCES_DIR})
    Format(client_lib ${CLIENT_SOURCES_DIR})
endif()
