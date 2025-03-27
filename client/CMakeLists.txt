set(CLIENT_SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB CLIENT_SOURCES ${CLIENT_SOURCES_DIR}/*.c)

add_executable(client ${CLIENT_SOURCES_DIR}/client.c ${CLIENT_SOURCES})

target_link_libraries(client PRIVATE network utility)
target_compile_options(client PRIVATE -Wall -Wextra -Wpedantic)
target_compile_definitions(client PRIVATE $<$<CONFIG:Debug>:DEBUG>)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    include(Format)
    Format(client ${CLIENT_SOURCES_DIR})
endif()
