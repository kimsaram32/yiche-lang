#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "yiche.h"

int main(void)
{
  printf("TOKENIZING\n");

  tokenizer_context_t *tokenizer = tokenizer_context_create(stdin);
  if (tokenizer == NULL)
    exit_out_of_memory();

  tokenizer_result_t tokenizer_result = tokenizer_process(tokenizer);
  if (tokenizer_result != TOKENIZER_SUCCESS)
  {
    tokenizer_print_error(tokenizer, tokenizer_result);
    exit(EXIT_FAILURE);
  }
  tokenizer_print_tokens(tokenizer);

  printf("PARSING\n");

  ast_node_t *root = NULL;
  parser_context_t *parser = parser_context_create(tokenizer);

  parser_result_t parser_result = parser_parse(parser, &root);
  if (parser_result != PARSER_SUCCESS)
  {
    parser_print_error(parser, parser_result);
    parser_context_free(parser);
    tokenizer_context_free(tokenizer);
    exit(EXIT_FAILURE);
  }
  ast_node_print(root);

  printf("EVALUATING\n");
  evaluate_value_t evaluator_result = evaluate(root);
  printf("returned %d\n", evaluator_result.value_int);

  parser_context_free(parser);
  tokenizer_context_free(tokenizer);

  return 0;
}
