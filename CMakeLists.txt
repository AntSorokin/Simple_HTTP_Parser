cmake_minimum_required(VERSION 3.14)
project(SIMPLE_HTTP_PARSER VERSION 1.0.0)

set(SIMPLE_HTTP_BUILD_TESTS 0)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libs/Array)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libs/HashMap)

add_library(SIMPLE_HTTP)
target_sources(SIMPLE_HTTP PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/headers.c ${CMAKE_CURRENT_SOURCE_DIR}/src/simple_http.c)
target_include_directories(SIMPLE_HTTP PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(SIMPLE_HTTP Array Hashmap)

if(SIMPLE_HTTP_BUILD_TESTS)
    add_subdirectory(tests)
endif()


