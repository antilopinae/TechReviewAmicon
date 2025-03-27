cmake_minimum_required(VERSION 3.20.0)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(CCache)

project(TechTask VERSION 1.0.0 LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS YES)

add_subdirectory(utility)

add_subdirectory(network)
add_subdirectory(server)
add_subdirectory(client)

message(STATUS "BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

# set(CMAKE_BUILD_TEST ON)

if(CMAKE_BUILD_TEST STREQUAL "ON")
    add_subdirectory(test)
endif()

install(TARGETS server client journal_utility
    RUNTIME DESTINATION bin
)
