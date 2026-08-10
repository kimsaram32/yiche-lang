add_executable(print_tokens "print_tokens.c")
target_link_libraries(print_tokens PRIVATE yiche)

add_executable(print_ast "print_ast.c")
target_link_libraries(print_ast PRIVATE yiche)

add_executable(run_evaluator "run_evaluator.c")
target_link_libraries(run_evaluator PRIVATE yiche)

find_program(BASH_PROGRAM bash REQUIRED)

add_custom_target(generate_tokenizer_outputs DEPENDS print_tokens)

add_custom_command(
  TARGET generate_tokenizer_outputs
  POST_BUILD
  COMMAND ${BASH_PROGRAM} ./tokenizer_generate_output.bash
          $<TARGET_FILE:print_tokens>
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  USES_TERMINAL VERBATIM)

add_custom_target(generate_parser_outputs DEPENDS print_ast)

add_custom_command(
  TARGET generate_parser_outputs
  POST_BUILD
  COMMAND ${BASH_PROGRAM} ./parser_generate_output.bash $<TARGET_FILE:print_ast>
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  USES_TERMINAL VERBATIM)

add_custom_target(generate_evaluator_outputs DEPENDS run_evaluator)

add_custom_command(
  TARGET generate_evaluator_outputs
  POST_BUILD
  COMMAND ${BASH_PROGRAM} ./evaluator_generate_output.bash
          $<TARGET_FILE:run_evaluator>
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  USES_TERMINAL VERBATIM)

add_custom_target(
  generate_outputs DEPENDS generate_tokenizer_outputs generate_parser_outputs
                           generate_evaluator_outputs)

add_test(
  NAME yiche_tests
  COMMAND ${BASH_PROGRAM} ./run_tests.bash $<TARGET_FILE:print_tokens>
          $<TARGET_FILE:print_ast> $<TARGET_FILE:run_evaluator>
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
