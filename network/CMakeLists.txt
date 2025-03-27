set(NETWORK_SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB NETWORK_SOURCES ${NETWORK_SOURCES_DIR}/*.c)

add_library(network STATIC ${NETWORK_SOURCES})

target_link_libraries(network PRIVATE utility)
target_include_directories(network PUBLIC ${NETWORK_SOURCES_DIR})
target_compile_definitions(network PRIVATE $<$<CONFIG:Debug>:DEBUG>)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    include(Format)
    Format(network ${NETWORK_SOURCES_DIR})
endif()
