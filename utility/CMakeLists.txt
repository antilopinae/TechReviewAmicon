set(UTILITY_SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB UTILITY_SOURCES ${UTILITY_SOURCES_DIR}/*.c)

add_library(utility STATIC ${UTILITY_SOURCES})

target_include_directories(utility PUBLIC ${UTILITY_SOURCES_DIR})

target_compile_definitions(utility PRIVATE $<$<CONFIG:Debug>:DEBUG>)