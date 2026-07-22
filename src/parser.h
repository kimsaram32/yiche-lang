#ifndef YICHE_PARSER_H
#define YICHE_PARSER_H

#include "tokenizer.h"

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

typedef enum {
  // ast_node_program_t
  AST_PROGRAM,

  /*
   * Declarations
   */

  AST_DECL_VARIABLE, // ast_node_variable_decl_t
  AST_DECL_FUNCTION, // ast_node__decl_t

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
} ast_node_type_t;

typedef enum
{
  DATA_TYPE_INT,
}
data_type_t;

// TODO: The relationship between these two members needs to be improved.
// Currently, it's too easy to make an invalid combination.

typedef struct
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
  ast_node_t *operand; // expression
}
ast_node_unary_expr_t;

#define DATA_BINARY_EXPR(node) ((ast_node_binary_expr_t*)((node)->data))

typedef struct
{
  ast_node_t *left_operand, *right_operand; // expression
}
ast_node_binary_expr_t;

#define DATA_FUNCTION_CALL_EXPR(node) ((ast_node_function_call_expr_t*)((node)->data))

typedef struct
{
  ast_node_t *callee; // expression
  VECTOR_T(ast_node_t*) *arguments; // expression
}
ast_node_function_call_expr_t;

#define DATA_STMT_LIST(node) ((ast_node_stmt_list_t*)((node)->data))

typedef struct
{
  VECTOR_T(ast_node_t*) *stmts; // ast_node_stmt_t
}
ast_node_stmt_list_t;

#define DATA_STMT(node) ((ast_node_stmt_t*)((node)->data))

typedef struct
{
  ast_node_t *stmt_list; // ast_node_stmt_list_t
  ast_node_t *expr_0; // expression
}
ast_node_stmt_t;

#define DATA_VARIABLE_DECL(node) ((ast_node_variable_decl_t*)((node)->data))

typedef struct
{
  token_t *token_identifier;
  data_type_t data_type;
  ast_node_t *initializer; // expression
}
ast_node_variable_decl_t;

#define DATA_FUNCTION_DECL(node) ((ast_node_function_decl_t*)((node)->data))

typedef struct
{
  token_t *token_identifier;
  VECTOR_T(ast_node_t*) *parameters; // ast_node_variable_decl_t
  data_type_t return_data_type;
  ast_node_t *body; // ast_node_stmt_list_t
}
ast_node_function_decl_t;

#define DATA_PROGRAM(node) ((ast_node_program_t*)((node)->data))

typedef struct
{
  VECTOR_T(ast_node_t*) *decls; // declaration
  data_type_t return_data_type;
}
ast_node_program_t;

ast_node_t *parse(void);

#endif
