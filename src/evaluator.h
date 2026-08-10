#ifndef YICHE_EVAULATOR_H
#define YICHE_EVAULATOR_H

#include "ast.h"

typedef enum
{
  EVAL_SUCCESS,
  EVAL_OOM,
  EVAL_RUNTIME_ERROR,
}
eval_result_type_t;

typedef struct
{
  data_type_t data_type;
  int value_int;
}
eval_value_t;

typedef struct
{
  eval_result_type_t type;
  union
  {
    eval_value_t value;
    char *error_message;
  };
}
eval_result_t;

// Evaluate the root node, which must have type 'AST_NODE_PROGRAM'.
//
// On success, 'type' is set to 'EVAL_SUCCESS', and 'value' contains the
// resulting evaluation value.
//
// On failure, 'type' is set to one of the error codes, and 'error_message'
// points to the error message. The result struct owns it, and callers are
// responsible for deallocation.
eval_result_t evaluate(ast_node_t *node_program);

#endif
