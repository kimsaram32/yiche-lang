#ifndef _PARSER_H
#define _PARSER_H

typedef enum
{
  // Declarations
  AST_DECL_VARIABLE,
  AST_DECL_FUNCTION,

  // Statements
  AST_STMT_EXPR,
  AST_STMT_IF,
  AST_STMT_WHILE,
  AST_STMT_RETURN,
  AST_STMT_LIST,

  // Expressions
  AST_EXPR_PRIMITIVE,

  AST_EXPR_FUNCTION_CALL,

  AST_EXPR_LOGICAL_NEGATION,

  AST_EXPR_ADDITION,
  AST_EXPR_SUBTRACTION,
  AST_EXPR_MULTIPLICATION,
  AST_EXPR_DIVISION,
  AST_EXPR_MODULO,

  AST_EXPR_LESS_THAN,
  AST_EXPR_GREATER_THAN,
  AST_EXPR_LESS_THAN_EQUAL_TO,
  AST_EXPR_GREATER_THAN_EQUAL_TO,

  AST_EXPR_EQUALS,
  AST_EXPR_NOT_EQUALS,

  AST_EXPR_LOGICAL_AND,
  AST_EXPR_LOGICAL_OR,

  AST_EXPR_ASSIGNMENT,
}
ast_node_type_t;

typedef struct ast_node_t
{
  ast_node_type_t type;
  void *data;
}
ast_node_t;

typedef struct
{
  // TODO: Proper handling
  token_t *token;
}
ast_node_primitive_expr_t;

typedef struct
{
  ast_node_t *operand;
}
ast_node_unary_expr_t;

typedef struct
{
  ast_node_t *left_operand, *right_operand;
}
ast_node_binary_expr_t;

typedef struct
{
  ast_node_t *callee;
  ast_node_t **arguments;
  int arguments_size, arguments_capacity;
}
ast_node_function_call_expr_t;

ast_node_t *parse(void);

#endif
