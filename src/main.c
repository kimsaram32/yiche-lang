#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "yiche.h"

int main(void)
{
  printf("TOKENIZING\n");
  tokenize(stdin);
  tokens_print();

  printf("PARSING\n");
  ast_node_t *root = parse();
  ast_node_print(root);

  printf("EVALUATING\n");
  evaluate_value_t result = evaluate(root);
  printf("returned %d\n", result.value_int);

  return 0;
}
