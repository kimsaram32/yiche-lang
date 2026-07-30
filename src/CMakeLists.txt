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

add_executable(yiche-driver)
set_target_properties(yiche-driver PROPERTIES OUTPUT_NAME "yiche")
target_sources(yiche-driver PRIVATE main.c)
target_link_libraries(yiche-driver PRIVATE yiche)
