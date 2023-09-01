cmake_minimum_required(VERSION 3.0.0)
project(armv8_a_study VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()

add_executable(armv8_a_study main.cpp)
add_subdirectory(tests)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
