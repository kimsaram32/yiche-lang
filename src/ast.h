#ifndef _YICHE_AST_H
#define _YICHE_AST_H

#include "tokenizer.h"

/*
 * Adding a new node data type:
 * - Add definitions in ast.c and ast.h
 * - Update 'ast_node_print' in ast.c
 */

/*
 * Adding a new node type:
 * - ast.h: Add a new member to the 'ast_node_type_t' enum
 * - Update the corresponding node data type
 * - parser.c: Add/update the corresponding parser function
 */

/*
 * Node types & node definition
 */

typedef enum {
  // ast_node_program_t
  AST_PROGRAM,

  /*
   * Declarations
   */

  // ast_node_variable_decl_t
  AST_DECL_VARIABLE,

  // ast_node_function_decl_t
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

/*
 * Node data types
 */

/*
 * primitive_expr
 */

#define DATA_PRIMITIVE_EXPR(node) ((ast_node_primitive_expr_t*)((node)->data))

typedef struct
{
  token_t *token;
}
ast_node_primitive_expr_t;

ast_node_t *ast_node_primitive_expr_create(token_t *token);

/*
 * unary_expr
 */

#define DATA_UNARY_EXPR(node) ((ast_node_unary_expr_t*)((node)->data))

typedef struct
{
  ast_node_t *operand; // expression
}
ast_node_unary_expr_t;

ast_node_t *ast_node_unary_expr_create(ast_node_type_t type, ast_node_t *operand);

/*
 * binary_expr
 */

#define DATA_BINARY_EXPR(node) ((ast_node_binary_expr_t*)((node)->data))

typedef struct
{
  ast_node_t *left_operand, *right_operand; // expression
}
ast_node_binary_expr_t;

ast_node_t *ast_node_binary_expr_create(ast_node_type_t type, ast_node_t *left_operand,
                                        ast_node_t *right_operand);

/*
 * function_call_expr
 */

#define DATA_FUNCTION_CALL_EXPR(node) ((ast_node_function_call_expr_t*)((node)->data))

typedef struct
{
  ast_node_t *callee; // expression
  VECTOR_T(ast_node_t*) *arguments; // expression
}
ast_node_function_call_expr_t;

ast_node_t *ast_node_function_call_expr_create(ast_node_t *callee);
void ast_node_function_call_expr_append_argument(ast_node_t *node, ast_node_t *arg);

/*
 * stmt_list
 */

#define DATA_STMT_LIST(node) ((ast_node_stmt_list_t*)((node)->data))

typedef struct
{
  VECTOR_T(ast_node_t*) *stmts; // ast_node_stmt_t
}
ast_node_stmt_list_t;

ast_node_t *ast_node_stmt_list_create(void);
void ast_node_stmt_list_append_stmt(ast_node_t *node, ast_node_t *stmt);

/*
 * stmt
 */

#define DATA_STMT(node) ((ast_node_stmt_t*)((node)->data))

typedef struct
{
  ast_node_t *stmt_list; // ast_node_stmt_list_t
  ast_node_t *expr_0; // expression
}
ast_node_stmt_t;

ast_node_t *ast_node_stmt_create(ast_node_type_t type, ast_node_t *stmt_list,
                                 ast_node_t *expr_0);

/*
 * variable_decl
 */

#define DATA_VARIABLE_DECL(node) ((ast_node_variable_decl_t*)((node)->data))

typedef struct
{
  token_t *token_identifier;
  data_type_t data_type;
  ast_node_t *initializer; // expression
}
ast_node_variable_decl_t;

ast_node_t *ast_node_variable_decl_create(token_t *token_identifier,
                                          data_type_t data_type, ast_node_t *initializer);

/*
 * function_decl
 */

#define DATA_FUNCTION_DECL(node) ((ast_node_function_decl_t*)((node)->data))

typedef struct
{
  token_t *token_identifier;
  VECTOR_T(ast_node_t*) *parameters; // ast_node_variable_decl_t
  data_type_t return_data_type;
  ast_node_t *body; // ast_node_stmt_list_t
}
ast_node_function_decl_t;

ast_node_t *ast_node_function_decl_create(token_t *token_identifier);
void ast_node_function_decl_append_parameter(ast_node_t *node, ast_node_t *parameter);

/*
 * program
 */

#define DATA_PROGRAM(node) ((ast_node_program_t*)((node)->data))

typedef struct
{
  VECTOR_T(ast_node_t*) *decls; // declaration
  data_type_t return_data_type;
}
ast_node_program_t;

ast_node_t *ast_node_program_create(void);
void ast_node_program_append_decl(ast_node_t *node, ast_node_t *decl);

/*
 * Printing
 */

void ast_node_print(ast_node_t *node);

#endif
