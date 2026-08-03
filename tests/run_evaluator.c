#include "yiche.h"

int main(void)
{
  tokenizer_context_t *tokenizer = tokenizer_context_create(stdin);
  if (tokenizer == NULL)
    exit_out_of_memory();

  tokenize(tokenizer);
  ast_node_t *root = parse(tokenizer);

  evaluate_value_t result = evaluate(root);
  printf("returned %d\n", result.value_int);

  tokenizer_context_free(tokenizer);
  return 0;
}
