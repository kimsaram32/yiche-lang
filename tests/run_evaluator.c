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

  ast_node_t *root;

  evaluate_value_t result = evaluate(root);
  printf("returned %d\n", result.value_int);

  tokenizer_context_free(tokenizer);
  return 0;
}
