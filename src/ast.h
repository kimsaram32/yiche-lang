#ifndef YICHE_AST_H
#define YICHE_AST_H

#include "tokenizer.h"

/*
 * Adding a new node type:
 * - Add definitions in ast.c and ast.h
 * - Update 'ast_node_print' in ast.c
 */

typedef enum {
  AST_NODE_PRIMITIVE_EXPR,
  AST_NODE_UNARY_EXPR,
  AST_NODE_BINARY_EXPR,
  AST_NODE_FUNCTION_CALL_EXPR,

  AST_NODE_STMT_LIST,
  AST_NODE_EXPR_STMT,
  AST_NODE_IF_STMT,
  AST_NODE_WHILE_STMT,
  AST_NODE_RETURN_STMT,

  AST_NODE_VARIABLE_DECL,
  AST_NODE_FUNCTION_DECL,

  AST_NODE_PROGRAM,
} ast_node_type_t;

typedef enum
{
  DATA_TYPE_INT,
}
data_type_t;

typedef struct ast_node_t ast_node_t;

/*
 * primitive_expr
 */

typedef struct
{
  token_t *token;
}
ast_node_primitive_expr_t;

ast_node_t *ast_node_primitive_expr_create(token_t *token);

/*
 * unary_expr
 */

typedef enum
{
  UNARY_OPERATOR_LOGICAL_NEGATION,
}
unary_operator_t;

char *unary_operator_to_string(unary_operator_t operator);

typedef struct
{
  unary_operator_t operator;
  ast_node_t *operand; // expression
}
ast_node_unary_expr_t;

ast_node_t *ast_node_unary_expr_create(unary_operator_t operator, ast_node_t *operand);

/*
 * binary_expr
 */

typedef enum
{
  BINARY_OPERATOR_ADDITION,
  BINARY_OPERATOR_SUBTRACTION,
  BINARY_OPERATOR_MULTIPLICATION,
  BINARY_OPERATOR_DIVISION,
  BINARY_OPERATOR_MODULO,

  BINARY_OPERATOR_LESS_THAN,
  BINARY_OPERATOR_GREATER_THAN,
  BINARY_OPERATOR_LESS_THAN_EQUAL_TO,
  BINARY_OPERATOR_GREATER_THAN_EQUAL_TO,

  BINARY_OPERATOR_EQUALS,
  BINARY_OPERATOR_NOT_EQUALS,

  BINARY_OPERATOR_LOGICAL_AND,
  BINARY_OPERATOR_LOGICAL_OR,

  BINARY_OPERATOR_ASSIGNMENT,
}
binary_operator_t;

char *binary_operator_to_string(binary_operator_t operator);

typedef struct
{
  binary_operator_t operator;
  ast_node_t *left_operand, *right_operand; // expression
}
ast_node_binary_expr_t;

ast_node_t *ast_node_binary_expr_create(binary_operator_t operator, ast_node_t *left_operand,
                                        ast_node_t *right_operand);

/*
 * function_call_expr
 */

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

typedef struct
{
  VECTOR_T(ast_node_t*) *stmts; // ast_node_stmt_t
}
ast_node_stmt_list_t;

ast_node_t *ast_node_stmt_list_create(void);
void ast_node_stmt_list_append_stmt(ast_node_t *node, ast_node_t *stmt);

/*
 * expr_stmt
 */

typedef struct
{
  ast_node_t *expr; // expression
}
ast_node_expr_stmt_t;

ast_node_t *ast_node_expr_stmt_create(ast_node_t *expr);

/*
 * if_stmt
 */

typedef struct
{
  ast_node_t *cond_expr; // expression
  ast_node_t *stmt_list;
}
ast_node_if_stmt_t;

ast_node_t *ast_node_if_stmt_create(ast_node_t *expr, ast_node_t *stmt_list);

/*
 * while_stmt
 */

typedef struct
{
  ast_node_t *cond_expr; // expression
  ast_node_t *stmt_list;
}
ast_node_while_stmt_t;

ast_node_t *ast_node_while_stmt_create(ast_node_t *cond_expr, ast_node_t *stmt_list);

/*
 * return_stmt
 */

typedef struct
{
  ast_node_t *expr; // expression
}
ast_node_return_stmt_t;

ast_node_t *ast_node_return_stmt_create(ast_node_t *expr);

/*
 * variable_decl
 */

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

typedef struct
{
  VECTOR_T(ast_node_t*) *decls; // declaration
}
ast_node_program_t;

ast_node_t *ast_node_program_create(void);
void ast_node_program_append_decl(ast_node_t *node, ast_node_t *decl);

/*
 * node definition
 */

typedef struct ast_node_t
{
  ast_node_type_t type;
  union
  {
    ast_node_primitive_expr_t data_primitive_expr;
    ast_node_unary_expr_t data_unary_expr;
    ast_node_binary_expr_t data_binary_expr;
    ast_node_function_call_expr_t data_function_call_expr;
    ast_node_stmt_list_t data_stmt_list;
    ast_node_expr_stmt_t data_expr_stmt;
    ast_node_if_stmt_t data_if_stmt;
    ast_node_while_stmt_t data_while_stmt;
    ast_node_return_stmt_t data_return_stmt;
    ast_node_variable_decl_t data_variable_decl;
    ast_node_function_decl_t data_function_decl;
    ast_node_program_t data_program;
  };
}
ast_node_t;

/*
 * Printing
 */

void ast_node_print(ast_node_t *node);

#endif
