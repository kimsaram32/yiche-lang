#ifndef _PARSER_H
#define _PARSER_H

/*
 * Adding a new node data type:
 * - parser.h: Define the struct and convenience macro
 * - parser.c: Define its creator function
 */

/*
 * Adding a new node type:
 * - parser.h: Add a member to the enum
 * - parser.c: Add/update a corresponding parser function
 */

typedef enum
{
  /*
   * Declarations
   */

  AST_DECL_VARIABLE,
  AST_DECL_FUNCTION,

  /*
   * Statements
   */

  // ast_node_stmt_t
  AST_STMT_EXPR, // only expr_0 is used
  AST_STMT_IF,
  AST_STMT_WHILE,
  AST_STMT_RETURN,

  // ast_node_stmt_list_t
  AST_STMT_LIST,

  /*
   * Expressions
   */

  // ast_node_primitive_expr_t
  AST_EXPR_PRIMITIVE,

  // ast_node_function_call_expr_t
  AST_EXPR_FUNCTION_CALL,

  // ast_node_unary_expr_t
  AST_EXPR_LOGICAL_NEGATION,

  // ast_node_binary_expr_t
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

// TODO: The relationship between these two members needs to be improved.
// Currently, it's too easy to make an invalid combination.

typedef struct ast_node_t
{
  ast_node_type_t type;
  void *data;
}
ast_node_t;

#define DATA_PRIMITIVE_EXPR(node) ((ast_node_primitive_expr_t*)((node)->data))

typedef struct
{
  // TODO: Proper handling
  token_t *token;
}
ast_node_primitive_expr_t;

#define DATA_UNARY_EXPR(node) ((ast_node_unary_expr_t*)((node)->data))

typedef struct
{
  ast_node_t *operand;
}
ast_node_unary_expr_t;

#define DATA_BINARY_EXPR(node) ((ast_node_binary_expr_t*)((node)->data))

typedef struct
{
  ast_node_t *left_operand, *right_operand;
}
ast_node_binary_expr_t;

#define DATA_FUNCTION_CALL_EXPR(node) ((ast_node_function_call_expr_t*)((node)->data))

typedef struct
{
  ast_node_t *callee;
  ast_node_t **arguments;
  int arguments_size, arguments_capacity;
}
ast_node_function_call_expr_t;

#define DATA_STMT_LIST(node) ((ast_node_stmt_list_t*)((node)->data))

typedef struct
{
  struct ast_node_t **stmts;
  int stmts_size, stmts_capacity;
}
ast_node_stmt_list_t;

#define DATA_STMT(node) ((ast_node_stmt_t*)((node)->data))

typedef struct
{
  ast_node_t *stmt_list, *expr_0;
}
ast_node_stmt_t;

ast_node_t *parse(void);

#endif
