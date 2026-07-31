add_executable(print_tokens "print_tokens.c")
target_link_libraries(print_tokens PRIVATE yiche)

add_executable(print_ast "print_ast.c")
target_link_libraries(print_ast PRIVATE yiche)

find_program(BASH_PROGRAM bash REQUIRED)

file(GLOB TOKENIZER_TEST_INPUT_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tokenizer/*.txt")
list(FILTER TOKENIZER_TEST_INPUT_FILES EXCLUDE REGEX "\\.expected\\.txt$")

file(GLOB PARSER_TEST_INPUT_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/parser/*.yiche")

add_custom_target(generate_tokenizer_outputs DEPENDS print_tokens)

foreach (TEST_INPUT_FILE IN LISTS TOKENIZER_TEST_INPUT_FILES)
  add_custom_command(
    TARGET generate_tokenizer_outputs POST_BUILD
    COMMAND ${BASH_PROGRAM} ./tokenizer_generate_output.bash
    $<TARGET_FILE:print_tokens> ${TEST_INPUT_FILE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    USES_TERMINAL
    VERBATIM
  )
endforeach ()

add_custom_target(generate_parser_outputs DEPENDS print_ast)

foreach (TEST_INPUT_FILE IN LISTS PARSER_TEST_INPUT_FILES)
  add_custom_command(
    TARGET generate_parser_outputs POST_BUILD
    COMMAND ${BASH_PROGRAM} ./parser_generate_output.bash
    $<TARGET_FILE:print_ast> ${TEST_INPUT_FILE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    USES_TERMINAL
    VERBATIM
  )
endforeach ()

add_custom_target(generate_outputs DEPENDS generate_tokenizer_outputs generate_parser_outputs)

add_test(
  NAME yiche_tests
  COMMAND ${BASH_PROGRAM} ./run_tests.bash ${CMAKE_BINARY_DIR}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
