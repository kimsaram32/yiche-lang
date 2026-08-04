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
  ast_node_t *root = parse(tokenizer);
  ast_node_print(root);

  printf("EVALUATING\n");
  evaluate_value_t evaluator_result = evaluate(root);
  printf("returned %d\n", evaluator_result.value_int);

  tokenizer_context_free(tokenizer);

  return 0;
}
