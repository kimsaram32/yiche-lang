#include "yiche.h"

int main(void)
{
  tokenize(stdin);
  ast_node_t *root = parse();

  evaluate_value_t result = evaluate(root);
  printf("returned %d\n", result.value_int);

  return 0;
}
