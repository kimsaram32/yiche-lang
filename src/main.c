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

  tokenize(tokenizer);
  tokens_print(tokenizer);

  printf("PARSING\n");
  ast_node_t *root = parse(tokenizer);
  ast_node_print(root);

  printf("EVALUATING\n");
  evaluate_value_t result = evaluate(root);
  printf("returned %d\n", result.value_int);

  tokenizer_context_free(tokenizer);

  return 0;
}
