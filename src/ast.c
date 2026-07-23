#include <stdlib.h>
#include "yiche.h"

static void _ast_node_print(ast_node_t *node, int level);

void ast_node_print(ast_node_t *node)
{
  _ast_node_print(node, 0);
}

static void print_indent(int level)
{
  for (int i = 0; i < level; i++)
    printf("  ");
}

static char *data_type_to_string(data_type_t data_type)
{
  switch (data_type)
  {
    case DATA_TYPE_INT:
      return "int";
  }
}

#define AST_NODE_DEFINE_PRINT_FUNCTION(name) \
  static void ast_node_ ## name ## _print(ast_node_t *node, int level) \
  { \
    printf("%s {\n", # name);

#define AST_NODE_DEFINE_PRINT_FUNCTION_END \
    print_indent(level); \
    printf("}\n"); \
  }

#define AST_NODE_PRINT_ATTR(label, fmt, ...) do { \
    print_indent(level + 1); \
    printf("%s = ", label); \
    printf(fmt "\n", __VA_ARGS__); \
  } \
  while (0)

#define AST_NODE_PRINT_CHILDREN(label, children_vector) do { \
    print_indent(level + 1); \
    printf("%s[%d] = {\n", label, children_vector->length); \
    for (int i = 0; i < children_vector->length; i++) \
    { \
      print_indent(level + 2); \
      _ast_node_print(VECTOR_ARR(children_vector, ast_node_t*)[i], level + 2); \
    } \
    print_indent(level + 1); \
    printf("}\n"); \
  } \
  while (0)

#define AST_NODE_PRINT_CHILD(label, child) do { \
    print_indent(level + 1); \
    printf("%s = ", label); \
    _ast_node_print(child, level + 1); \
  } \
  while (0)

static ast_node_t *ast_node_create(ast_node_type_t type)
{
  ast_node_t *node = malloc(sizeof(ast_node_t));
  if (node == NULL)
    exit_out_of_memory();

  node->type = type;

  return node;
}

/*
 * primitive_expr
 */

ast_node_t *ast_node_primitive_expr_create(token_t *token)
{
  ast_node_t *node = ast_node_create(AST_NODE_PRIMITIVE_EXPR);

  node->data_primitive_expr.token = token;

  return node;
}

AST_NODE_DEFINE_PRINT_FUNCTION(primitive_expr)
{
  switch (node->data_primitive_expr.token->type)
  {
    case TOKEN_IDENTIFIER:
      AST_NODE_PRINT_ATTR("identifier", "%s", node->data_primitive_expr.token->identifier);
      break;
    case TOKEN_CONSTANT:
      AST_NODE_PRINT_ATTR("constant", "%d", node->data_primitive_expr.token->constant);
      break;
    default:
      UNREACHABLE;
  }
}
AST_NODE_DEFINE_PRINT_FUNCTION_END

/*
 * unary_expr
 */

ast_node_t *ast_node_unary_expr_create(unary_operator_t operator, ast_node_t *operand)
{
  ast_node_t *node = ast_node_create(AST_NODE_UNARY_EXPR);

  node->data_unary_expr.operator = operator;
  node->data_unary_expr.operand = operand;

  return node;
}

char *unary_operator_to_string(unary_operator_t operator)
{
  switch (operator)
  {
    case UNARY_OPERATOR_LOGICAL_NEGATION:
      return "!";
  }
}

AST_NODE_DEFINE_PRINT_FUNCTION(unary_expr)
{
  AST_NODE_PRINT_ATTR("operator", "%s", unary_operator_to_string(node->data_unary_expr.operator));
  AST_NODE_PRINT_CHILD("operand", node->data_unary_expr.operand);
}
AST_NODE_DEFINE_PRINT_FUNCTION_END

/*
 * binary_expr
 */

ast_node_t *ast_node_binary_expr_create(binary_operator_t operator, ast_node_t *left_operand,
                                        ast_node_t *right_operand)
{
  ast_node_t *node = ast_node_create(AST_NODE_BINARY_EXPR);

  node->data_binary_expr.operator = operator;
  node->data_binary_expr.left_operand = left_operand;
  node->data_binary_expr.right_operand = right_operand;

  return node;
}

char *binary_operator_to_string(binary_operator_t operator)
{
  switch (operator)
  {
    case BINARY_OPERATOR_ADDITION:
      return "+";
    case BINARY_OPERATOR_SUBTRACTION:
      return "-";
    case BINARY_OPERATOR_MULTIPLICATION:
      return "*";
    case BINARY_OPERATOR_DIVISION:
      return "/";
    case BINARY_OPERATOR_MODULO:
      return "%";
    case BINARY_OPERATOR_LESS_THAN:
      return "<";
    case BINARY_OPERATOR_GREATER_THAN:
      return ">";
    case BINARY_OPERATOR_LESS_THAN_EQUAL_TO:
      return "<=";
    case BINARY_OPERATOR_GREATER_THAN_EQUAL_TO:
      return ">=";
    case BINARY_OPERATOR_EQUALS:
      return "==";
    case BINARY_OPERATOR_NOT_EQUALS:
      return "!=";
    case BINARY_OPERATOR_LOGICAL_AND:
      return "&&";
    case BINARY_OPERATOR_LOGICAL_OR:
      return "||";
    case BINARY_OPERATOR_ASSIGNMENT:
      return "=";
  }
}

AST_NODE_DEFINE_PRINT_FUNCTION(binary_expr)
{
  AST_NODE_PRINT_ATTR("operator", "%s", binary_operator_to_string(node->data_binary_expr.operator));
  AST_NODE_PRINT_CHILD("left", node->data_binary_expr.left_operand);
  AST_NODE_PRINT_CHILD("right", node->data_binary_expr.right_operand);
}
AST_NODE_DEFINE_PRINT_FUNCTION_END

/*
 * function_call_expr
 */

ast_node_t *ast_node_function_call_expr_create(token_t *token_callee)
{
  ast_node_t *node = ast_node_create(AST_NODE_FUNCTION_CALL_EXPR);

  node->data_function_call_expr.token_callee = token_callee;
  node->data_function_call_expr.arguments = vector_pointer_create(8);
  if (node->data_function_call_expr.arguments == NULL)
    exit_out_of_memory();

  return node;
}

void ast_node_function_call_expr_append_argument(ast_node_t *node, ast_node_t *arg)
{
  if (!vector_pointer_append(node->data_function_call_expr.arguments, arg))
    exit_out_of_memory();
}

AST_NODE_DEFINE_PRINT_FUNCTION(function_call_expr)
{
  AST_NODE_PRINT_ATTR("callee", "%s", node->data_function_call_expr.token_callee->identifier);
  AST_NODE_PRINT_CHILDREN("arguments", node->data_function_call_expr.arguments);
}
AST_NODE_DEFINE_PRINT_FUNCTION_END

/*
 * stmt_list
 */

ast_node_t *ast_node_stmt_list_create(void)
{
  ast_node_t *node = ast_node_create(AST_NODE_STMT_LIST);

  node->data_stmt_list.stmts = vector_pointer_create(8);
  if (node->data_stmt_list.stmts == NULL)
    exit_out_of_memory();

  return node;
}

void ast_node_stmt_list_append_stmt(ast_node_t *node, ast_node_t *stmt)
{
  if (!vector_pointer_append(node->data_stmt_list.stmts, stmt))
    exit_out_of_memory();
}

AST_NODE_DEFINE_PRINT_FUNCTION(stmt_list)
{
  AST_NODE_PRINT_CHILDREN("stmts", node->data_stmt_list.stmts);
}
AST_NODE_DEFINE_PRINT_FUNCTION_END

/*
 * expr_stmt
 */

ast_node_t *ast_node_expr_stmt_create(ast_node_t *expr)
{
  ast_node_t *node = ast_node_create(AST_NODE_EXPR_STMT);

  node->data_expr_stmt.expr = expr;

  return node;
}

AST_NODE_DEFINE_PRINT_FUNCTION(expr_stmt)
{
  AST_NODE_PRINT_CHILD("expr", node->data_expr_stmt.expr);
}
AST_NODE_DEFINE_PRINT_FUNCTION_END

/*
 * if_stmt
 */

ast_node_t *ast_node_if_stmt_create(ast_node_t *cond_expr, ast_node_t *stmt_list)
{
  ast_node_t *node = ast_node_create(AST_NODE_IF_STMT);

  node->data_if_stmt.cond_expr = cond_expr;
  node->data_if_stmt.stmt_list = stmt_list;

  return node;
}

AST_NODE_DEFINE_PRINT_FUNCTION(if_stmt)
{
  AST_NODE_PRINT_CHILD("expr", node->data_if_stmt.cond_expr);
  AST_NODE_PRINT_CHILD("stmt_list", node->data_if_stmt.stmt_list);
}
AST_NODE_DEFINE_PRINT_FUNCTION_END

/*
 * while_stmt
 */

ast_node_t *ast_node_while_stmt_create(ast_node_t *cond_expr, ast_node_t *stmt_list)
{
  ast_node_t *node = ast_node_create(AST_NODE_WHILE_STMT);

  node->data_while_stmt.cond_expr = cond_expr;
  node->data_while_stmt.stmt_list = stmt_list;

  return node;
}

AST_NODE_DEFINE_PRINT_FUNCTION(while_stmt)
{
  AST_NODE_PRINT_CHILD("expr", node->data_while_stmt.cond_expr);
  AST_NODE_PRINT_CHILD("stmt_list", node->data_while_stmt.stmt_list);
}
AST_NODE_DEFINE_PRINT_FUNCTION_END

/*
 * return_stmt
 */

ast_node_t *ast_node_return_stmt_create(ast_node_t *expr)
{
  ast_node_t *node = ast_node_create(AST_NODE_RETURN_STMT);

  node->data_return_stmt.expr = expr;

  return node;
}

AST_NODE_DEFINE_PRINT_FUNCTION(return_stmt)
{
  AST_NODE_PRINT_CHILD("expr", node->data_return_stmt.expr);
}
AST_NODE_DEFINE_PRINT_FUNCTION_END

/*
 * variable_decl
 */

ast_node_t *ast_node_variable_decl_create(token_t *token_identifier,
                                          data_type_t data_type, ast_node_t *initializer)
{
  ast_node_t *node = ast_node_create(AST_NODE_VARIABLE_DECL);

  node->data_variable_decl.token_identifier = token_identifier;
  node->data_variable_decl.data_type = data_type;
  node->data_variable_decl.initializer = initializer;

  return node;
}

AST_NODE_DEFINE_PRINT_FUNCTION(variable_decl)
{
  AST_NODE_PRINT_ATTR("identifier", "%s", node->data_variable_decl.token_identifier->identifier);
  AST_NODE_PRINT_ATTR("data_type", "%s", data_type_to_string(node->data_variable_decl.data_type));

  if (node->data_variable_decl.initializer != NULL)
    AST_NODE_PRINT_CHILD("initializer", node->data_variable_decl.initializer);
}
AST_NODE_DEFINE_PRINT_FUNCTION_END

/*
 * function_decl
 */

ast_node_t *ast_node_function_decl_create(token_t *token_identifier)
{
  ast_node_t *node = ast_node_create(AST_NODE_FUNCTION_DECL);

  node->data_function_decl.token_identifier = token_identifier;
  node->data_function_decl.parameters = vector_pointer_create(8);
  if (node->data_function_decl.parameters == NULL)
    exit_out_of_memory();

  return node;
}

void ast_node_function_decl_append_parameter(ast_node_t *node, ast_node_t *parameter)
{
  if (!vector_pointer_append(node->data_function_decl.parameters, parameter))
    exit_out_of_memory();
}

AST_NODE_DEFINE_PRINT_FUNCTION(function_decl)
{
  AST_NODE_PRINT_ATTR("identifier", "%s", node->data_function_decl.token_identifier->identifier);
  AST_NODE_PRINT_ATTR("return_data_type", "%s",
                      data_type_to_string(node->data_function_decl.return_data_type));
  AST_NODE_PRINT_CHILDREN("parameters", node->data_function_decl.parameters);
  AST_NODE_PRINT_CHILD("body", node->data_function_decl.body);
}
AST_NODE_DEFINE_PRINT_FUNCTION_END

/*
 * program
 */

ast_node_t *ast_node_program_create(void)
{
  ast_node_t *node = ast_node_create(AST_NODE_PROGRAM);

  node->data_program.decls = vector_pointer_create(8);
  if (node->data_program.decls == NULL)
    exit_out_of_memory();

  return node;
}

void ast_node_program_append_decl(ast_node_t *node, ast_node_t *decl)
{
  if (!vector_pointer_append(node->data_program.decls, decl))
    exit_out_of_memory();
}

AST_NODE_DEFINE_PRINT_FUNCTION(program)
{
  AST_NODE_PRINT_CHILDREN("decls", node->data_program.decls);
}
AST_NODE_DEFINE_PRINT_FUNCTION_END

/*
 * Printing
 */

void _ast_node_print(ast_node_t *node, int level)
{
  switch (node->type)
  {
    case AST_NODE_PRIMITIVE_EXPR:
      ast_node_primitive_expr_print(node, level);
      break;
    case AST_NODE_UNARY_EXPR:
      ast_node_unary_expr_print(node, level);
      break;
    case AST_NODE_BINARY_EXPR:
      ast_node_binary_expr_print(node, level);
      break;
    case AST_NODE_FUNCTION_CALL_EXPR:
      ast_node_function_call_expr_print(node, level);
      break;
    case AST_NODE_STMT_LIST:
      ast_node_stmt_list_print(node, level);
      break;
    case AST_NODE_EXPR_STMT:
      ast_node_expr_stmt_print(node, level);
      break;
    case AST_NODE_IF_STMT:
      ast_node_if_stmt_print(node, level);
      break;
    case AST_NODE_WHILE_STMT:
      ast_node_while_stmt_print(node, level);
      break;
    case AST_NODE_RETURN_STMT:
      ast_node_return_stmt_print(node, level);
      break;
    case AST_NODE_VARIABLE_DECL:
      ast_node_variable_decl_print(node, level);
      break;
    case AST_NODE_FUNCTION_DECL:
      ast_node_function_decl_print(node, level);
      break;
    case AST_NODE_PROGRAM:
      ast_node_program_print(node, level);
      break;
  }
}
