cmake_minimum_required(VERSION 3.23)
set(CMAKE_C_STANDARD 11)

project(yiche LANGUAGES C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
