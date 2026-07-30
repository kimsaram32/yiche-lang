#include "yiche.h"

int main(void)
{
  input_init();

  tokenize();

  ast_node_t *root = parse();
  ast_node_print(root);

  return 0;
}
