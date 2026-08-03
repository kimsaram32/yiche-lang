#include "yiche.h"

int main(void)
{
  tokenizer_context_t *tokenizer = tokenizer_context_create(stdin);
  if (tokenizer == NULL)
    exit_out_of_memory();

  tokenize(tokenizer);
  tokens_print(tokenizer);

  tokenizer_context_free(tokenizer);
  return 0;
}
