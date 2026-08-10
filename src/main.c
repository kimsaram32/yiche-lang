#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "yiche.h"

int main(void)
{
  tokenizer_context_t *tokenizer = NULL;
  parser_context_t *parser = NULL;
  ast_node_t *root = NULL;

  printf("TOKENIZING\n");

  tokenizer = tokenizer_context_create(stdin);
  if (tokenizer == NULL)
    exit_out_of_memory();

  tokenizer_result_t tokenizer_result = tokenizer_process(tokenizer);
  if (tokenizer_result != TOKENIZER_SUCCESS)
  {
    tokenizer_print_error(tokenizer, tokenizer_result);
    goto fail;
  }

  tokenizer_print_tokens(tokenizer);

  printf("PARSING\n");

  parser = parser_context_create(tokenizer);
  if (parser == NULL)
    exit_out_of_memory();

  parser_result_t parser_result = parser_parse(parser, &root);
  if (parser_result != PARSER_SUCCESS)
  {
    parser_print_error(parser, parser_result);
    goto fail;
  }

  ast_node_print(root);

  printf("EVALUATING\n");

  eval_result_t evaluator_result = evaluate(root);
  if (evaluator_result.type != EVAL_SUCCESS)
  {
    fprintf(stderr, "%s", evaluator_result.error_message);
    free(evaluator_result.error_message);
    goto fail;
  }

  printf("returned %d\n", evaluator_result.value.value_int);

  ast_node_free(root);
  parser_context_free(parser);
  tokenizer_context_free(tokenizer);

  return 0;

  fail: {
    if (root != NULL)
      ast_node_free(root);

    if (parser != NULL)
      parser_context_free(parser);

    if (tokenizer != NULL)
      tokenizer_context_free(tokenizer);

    exit(EXIT_FAILURE);
  }
}
