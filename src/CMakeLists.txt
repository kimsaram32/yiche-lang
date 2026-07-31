add_library(yiche STATIC)
target_sources(
  yiche
  PRIVATE error.c
          vector.c
          hash_table.c
          character.c
          input.c
          ast.c
          tokenizer.c
          parser.c
          evaluator.c
  PUBLIC FILE_SET headers TYPE HEADERS FILES yiche.h)

add_executable(yiche_driver)
set_target_properties(yiche_driver PROPERTIES OUTPUT_NAME "yiche")
target_sources(yiche_driver PRIVATE main.c)
target_link_libraries(yiche_driver PRIVATE yiche)
