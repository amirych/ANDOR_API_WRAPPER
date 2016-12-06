project(ANDOR_API_WRAPPER)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 11)


set(ANDOR_API_WRAPPER_LIB andor_api_wrapper)
aux_source_directory(./camera/ ANDOR_API_WRAPPER_SRC)
add_library(${ANDOR_API_WRAPPER_LIB} SHARED ${ANDOR_API_WRAPPER_SRC})


set(TEST_PROG test_prog)
add_executable(${TEST_PROG} test.cpp)
target_link_libraries(${TEST_PROG} ${ANDOR_API_WRAPPER_LIB})
