project(ANDOR_API_WRAPPER)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_BUILD_TYPE "Debug")

set (ANDOR_CAMERA_VERSION_MAJOR 0)
set (ANDOR_CAMERA_VERSION_MINOR 1)

configure_file (
  "./camera/andor_camera_config.h.in"
  "${PROJECT_BINARY_DIR}/andor_camera_config.h"
  )


set(ANDOR_API_WRAPPER_LIB andor_api_wrapper)
aux_source_directory(./camera/ ANDOR_API_WRAPPER_SRC)
include_directories(./camera)
add_library(${ANDOR_API_WRAPPER_LIB} SHARED ${ANDOR_API_WRAPPER_SRC})
target_link_libraries(${ANDOR_API_WRAPPER_LIB} atcore)


set(TEST_PROG test_prog)
add_executable(${TEST_PROG} test.cpp)
target_link_libraries(${TEST_PROG} ${ANDOR_API_WRAPPER_LIB})


install (TARGETS ${ANDOR_API_WRAPPER_LIB} DESTINATION lib)
install (TARGETS ${TEST_PROG} DESTINATION bin)
