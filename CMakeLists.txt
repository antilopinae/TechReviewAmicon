cmake_minimum_required(VERSION 3.20.0)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(CCache)

project(TechTask VERSION 1.0.0 LANGUAGES C)

enable_testing()

# set(CMAKE_THREAD_LIBS_INIT "-lpthread")
# set(CMAKE_HAVE_THREADS_LIBRARY 1)
# set(CMAKE_USE_WIN32_THREADS_INIT 0)
# set(CMAKE_USE_PTHREADS_INIT 1)
# set(THREADS_PREFER_PTHREAD_FLAG ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS YES)

add_executable(Server Server.c)
add_executable(Client Client.c)
add_executable(JournalUtility Logger.c)

target_compile_options(Server PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(Client PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(JournalUtility PRIVATE -Wall -Wextra -Wpedantic)

message(STATUS "BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

target_compile_definitions(Server PRIVATE $<$<CONFIG:Debug>:DEBUG>)
target_compile_definitions(Client PRIVATE $<$<CONFIG:Debug>:DEBUG>)
target_compile_definitions(JournalUtility PRIVATE $<$<CONFIG:Debug>:DEBUG>)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    include(Format)
    include(Memcheck)

    Format(Server ${CMAKE_CURRENT_LIST_DIR})
    Format(Client ${CMAKE_CURRENT_LIST_DIR})
    Format(JournalUtility ${CMAKE_CURRENT_LIST_DIR})

    AddMemcheck(Server)
    AddMemcheck(Client)
    AddMemcheck(JournalUtility)
endif()

install(TARGETS Server Client JournalUtility
    RUNTIME DESTINATION bin
)
