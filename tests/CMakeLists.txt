add_executable(print_tokens "print_tokens.c")

target_link_libraries(print_tokens PRIVATE yiche)

find_program(BASH_PROGRAM bash REQUIRED)

add_test(
  NAME yiche_tests
  COMMAND ${BASH_PROGRAM} ./run_tests.bash ${CMAKE_BINARY_DIR}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
