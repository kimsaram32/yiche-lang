#include <stdlib.h>
#include "yiche.h"

int main(void)
{
  tokenizer_context_t *tokenizer = tokenizer_context_create(stdin);
  if (tokenizer == NULL)
    exit_out_of_memory();

  if (tokenizer_process(tokenizer) != TOKENIZER_SUCCESS)
  {
    tokenizer_context_free(tokenizer);
    exit(EXIT_FAILURE);
  }

  ast_node_t *root = NULL;
  parser_context_t *parser = parser_context_create(tokenizer);
  if (parser == NULL)
    exit_out_of_memory();

  parser_result_t parser_result = parser_parse(parser, &root);
  if (parser_result != PARSER_SUCCESS)
  {
    parser_print_error(parser, parser_result);
    parser_context_free(parser);
    tokenizer_context_free(tokenizer);
    exit(EXIT_FAILURE);
  }

  ast_node_print(root);

  parser_context_free(parser);
  tokenizer_context_free(tokenizer);
  return 0;
}
