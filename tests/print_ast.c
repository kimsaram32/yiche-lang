#include "yiche.h"

int main(void)
{
  tokenize(stdin);

  ast_node_t *root = parse();
  ast_node_print(root);

  return 0;
}
