#ifndef YICHE_EVAULATOR_H
#define YICHE_EVAULATOR_H

#include "ast.h"

typedef struct
{
  data_type_t data_type;
  int value_int;
}
evaluate_value_t;

evaluate_value_t evaluate(ast_node_t *node_program);

#endif
