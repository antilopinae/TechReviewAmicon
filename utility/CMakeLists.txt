set(UTILITY_SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_library(utility INTERFACE ${UTILITY_SOURCES_DIR}/utility.h)

target_include_directories(utility INTERFACE ${UTILITY_SOURCES_DIR})