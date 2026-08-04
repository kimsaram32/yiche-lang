#include "yiche.h"

int main(void)
{
  tokenizer_context_t *tokenizer = tokenizer_context_create(stdin);
  if (tokenizer == NULL)
    exit_out_of_memory();

  tokenizer_result_t tokenizer_result = tokenize(tokenizer);
  if (tokenizer_result != TOKENIZER_SUCCESS)
  {
    tokenizer_print_error(tokenizer, tokenizer_result);
    exit(EXIT_FAILURE);
  }
  tokens_print(tokenizer);

  tokenizer_context_free(tokenizer);
  return 0;
}
