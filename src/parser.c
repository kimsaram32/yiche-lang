#include "yiche.h"

/*
 * Nodes
 */

static ast_node_t *ast_node_create(ast_node_type_t type)
{
  ast_node_t *node = malloc(sizeof(ast_node_t));
  if (node == NULL)
    exit_out_of_memory();

  node->type = type;
  node->data = NULL;

  return node;
}

#define AST_NODE_CREATE_WITH_DATA(node, data, type, data_type) do { \
    node = ast_node_create(type); \
    data = malloc(sizeof(data_type)); \
    if (data == NULL) {\
      exit_out_of_memory(); \
    } \
    node->data = data; \
  } \
  while (0)

static ast_node_t *ast_node_binary_expr_create(ast_node_type_t type, ast_node_t *left_operand,
                                               ast_node_t *right_operand)
{
  ast_node_t *node;
  ast_node_binary_expr_t *data;
  AST_NODE_CREATE_WITH_DATA(node, data, type, ast_node_binary_expr_t);

  data->left_operand = left_operand;
  data->right_operand = right_operand;

  return node;
}

static ast_node_t *ast_node_primitive_expr_create(token_t *token)
{
  ast_node_t *node;
  ast_node_primitive_expr_t *data;
  AST_NODE_CREATE_WITH_DATA(node, data, AST_EXPR_PRIMITIVE, ast_node_primitive_expr_t);

  data->token = token;

  return node;
}

static ast_node_t *ast_node_unary_expr_create(ast_node_type_t type, ast_node_t *operand)
{
  ast_node_t *node;
  ast_node_unary_expr_t *data;
  AST_NODE_CREATE_WITH_DATA(node, data, type, ast_node_unary_expr_t);

  data->operand = operand;

  return node;
}

static ast_node_t *ast_node_function_call_expr_create(ast_node_t *callee)
{
  ast_node_t *node;
  ast_node_function_call_expr_t *data;
  AST_NODE_CREATE_WITH_DATA(node, data, AST_EXPR_FUNCTION_CALL, ast_node_function_call_expr_t);

  data->callee = callee;
  data->arguments = vector_pointer_create(8);
  if (data->arguments == NULL)
    exit_out_of_memory();

  return node;
}

static void ast_node_function_call_expr_append_argument(ast_node_t *node, ast_node_t *arg)
{
  if (!vector_pointer_append(DATA_FUNCTION_CALL_EXPR(node)->arguments, arg))
    exit_out_of_memory();
}

static ast_node_t *ast_node_stmt_list_create(void)
{
  ast_node_t *node;
  ast_node_stmt_list_t *data;
  AST_NODE_CREATE_WITH_DATA(node, data, AST_STMT_LIST, ast_node_stmt_list_t);

  data->stmts = vector_pointer_create(8);
  if (data->stmts == NULL)
    exit_out_of_memory();

  return node;
}

static void ast_node_stmt_list_append_stmt(ast_node_t *node, ast_node_t *stmt)
{
  if (!vector_pointer_append(DATA_STMT_LIST(node)->stmts, stmt))
    exit_out_of_memory();
}

static ast_node_t *ast_node_stmt_create(ast_node_type_t type, ast_node_t *stmt_list,
                                        ast_node_t *expr_0)
{
  ast_node_t *node;
  ast_node_stmt_t *data;
  AST_NODE_CREATE_WITH_DATA(node, data, type, ast_node_stmt_t);

  data->stmt_list = stmt_list;
  data->expr_0 = expr_0;

  return node;
}

static ast_node_t *ast_node_variable_decl_create(token_t *token_identifier,
                                                 data_type_t data_type, ast_node_t *initializer)
{
  ast_node_t *node;
  ast_node_variable_decl_t *data;
  AST_NODE_CREATE_WITH_DATA(node, data, AST_DECL_VARIABLE, ast_node_variable_decl_t);

  data->token_identifier = token_identifier;
  data->data_type = data_type;
  data->initializer = initializer;

  return node;
}

static ast_node_t *ast_node_function_decl_create(token_t *token_identifier)
{
  ast_node_t *node;
  ast_node_function_decl_t *data;
  AST_NODE_CREATE_WITH_DATA(node, data, AST_DECL_FUNCTION, ast_node_function_decl_t);

  data->token_identifier = token_identifier;
  data->parameters = vector_pointer_create(8);
  if (data->parameters == NULL)
    exit_out_of_memory();

  return node;
}

static void ast_node_function_decl_append_parameter(ast_node_t *node, ast_node_t *parameter)
{
  if (!vector_pointer_append(DATA_FUNCTION_DECL(node)->parameters, parameter))
    exit_out_of_memory();
}

static ast_node_t *ast_node_program_create(void)
{
  ast_node_t *node;
  ast_node_program_t *data;
  AST_NODE_CREATE_WITH_DATA(node, data, AST_PROGRAM, ast_node_program_t);

  data->decls = vector_pointer_create(8);
  if (data->decls == NULL)
    exit_out_of_memory();

  return node;
}

static void ast_node_program_append_decl(ast_node_t *node, ast_node_t *decl)
{
  if (!vector_pointer_append(DATA_PROGRAM(node)->decls, decl))
    exit_out_of_memory();
}

/*
 * Parser functions
 */

/*
 * Writing a parser function for a new binary operator:
 * 1. Add a function declaration below
 * 2. Add new mapping in 'get_binary_operator_type'
 * 3. Implement the function by either using a macro or manually writing it
 */

static ast_node_t *parse_expr(void);
static ast_node_t *parse_primitive_expr(void);
static ast_node_t *parse_function_call_expr(void);
static ast_node_t *parse_prefix_expr(void);
static ast_node_t *parse_multiplicative_expr(void);
static ast_node_t *parse_additive_expr(void);
static ast_node_t *parse_comparison_expr(void);
static ast_node_t *parse_equality_expr(void);
static ast_node_t *parse_logical_and_expr(void);
static ast_node_t *parse_logical_or_expr(void);
static ast_node_t *parse_assignment_expr(void);

static ast_node_t *parse_stmt_list(void);
static ast_node_t *parse_expr_stmt(void);
static ast_node_t *parse_if_stmt(void);
static ast_node_t *parse_while_stmt(void);
static ast_node_t *parse_return_stmt(void);

static ast_node_t *parse_variable_decl(void);
static ast_node_t *parse_function_decl(void);

/*
 * Expressions
 */

static ast_node_t *parse_expr(void)
{
  return parse_assignment_expr();
}

static ast_node_t *parse_primitive_expr(void)
{
  token_t *token = token_advance_and_assert(3, TOKEN_IDENTIFIER, TOKEN_CONSTANT,
                                            TOKEN_SYMBOL_LPAREN);
  ast_node_t *node;

  if (token->type == TOKEN_SYMBOL_LPAREN)
  {
    node = parse_expr();
    token_advance_and_assert(1, TOKEN_SYMBOL_RPAREN);
    return node;
  }

  node = ast_node_primitive_expr_create(token);
  return node;
}

// <function_call_expression> ::= <primitive_expression> { "(" [ <function_call_argument_list> ] ")" }

static ast_node_t *parse_function_call_expr(void)
{
  ast_node_t *root = parse_primitive_expr();

  while (token_try_advancing(1, TOKEN_SYMBOL_LPAREN))
  {
    root = ast_node_function_call_expr_create(root);

    if (token_try_advancing(1, TOKEN_SYMBOL_RPAREN) == NULL)
    {
      do
        ast_node_function_call_expr_append_argument(root, parse_expr());
      while (token_advance_and_assert(2, TOKEN_SYMBOL_COMMA, TOKEN_SYMBOL_RPAREN)->type
             == TOKEN_SYMBOL_COMMA);
    }
  }

  return root;
}

static ast_node_t *parse_prefix_expr(void)
{
  ast_node_t *root = NULL, **node_p = &root;

  token_t *token_operator;

  while ((token_operator = token_try_advancing(1, TOKEN_SYMBOL_BANG)) != NULL)
  {
    ast_node_type_t type;
    switch (token_operator->type)
    {
      case TOKEN_SYMBOL_BANG:
        type = AST_EXPR_LOGICAL_NEGATION;
        break;
      default:
        break;
    }
    *node_p = ast_node_unary_expr_create(type, NULL);
    node_p = &(DATA_UNARY_EXPR(*node_p))->operand;
  }

  *node_p = parse_function_call_expr();
  return root;
}

static ast_node_type_t get_binary_operator_type(token_type_t symbol_type)
{
  switch (symbol_type)
  {
    case TOKEN_SYMBOL_ASTERISK:
      return AST_EXPR_MULTIPLICATION;
    case TOKEN_SYMBOL_SLASH:
      return AST_EXPR_DIVISION;
    case TOKEN_SYMBOL_PERCENT:
      return AST_EXPR_MODULO;

    case TOKEN_SYMBOL_PLUS:
      return AST_EXPR_ADDITION;
    case TOKEN_SYMBOL_MINUS:
      return AST_EXPR_SUBTRACTION;

    case TOKEN_SYMBOL_LT:
      return AST_EXPR_LESS_THAN;
    case TOKEN_SYMBOL_GT:
      return AST_EXPR_GREATER_THAN;
    case TOKEN_SYMBOL_LE:
      return AST_EXPR_LESS_THAN_EQUAL_TO;
    case TOKEN_SYMBOL_GE:
      return AST_EXPR_GREATER_THAN_EQUAL_TO;

    case TOKEN_SYMBOL_EQEQ:
      return AST_EXPR_EQUALS;
    case TOKEN_SYMBOL_BANGEQ:
      return AST_EXPR_NOT_EQUALS;

    case TOKEN_SYMBOL_ANDAND:
      return AST_EXPR_LOGICAL_AND;

    case TOKEN_SYMBOL_OROR:
      return AST_EXPR_LOGICAL_OR;

    case TOKEN_SYMBOL_EQ:
      return AST_EXPR_ASSIGNMENT;

    default:
      exit_with_error("get_binary_operator_type: missing binary operator mapping for symbol %d", symbol_type);
  }
}

// Left-recursive binary expressions are handled as right-recursive, e.g.,
// <multiplicative_expression> ::= <prefix_expression> [ ( "*" | "/" | "%" ) <multiplicative_expression> ]

#define DEFINE_BINARY_EXPR_PARSE_FUNCTION(func, prev, types_n, ...) \
static ast_node_t *func(void) \
{ \
  ast_node_t *root = prev(); \
  token_t *token_operator; \
  while ((token_operator = token_try_advancing(types_n, __VA_ARGS__)) != NULL) \
  { \
    ast_node_t *right_operand = prev(); \
    ast_node_type_t type = get_binary_operator_type(token_operator->type); \
    root = ast_node_binary_expr_create(type, root, right_operand); \
  } \
  return root; \
}

DEFINE_BINARY_EXPR_PARSE_FUNCTION(parse_multiplicative_expr, parse_prefix_expr, 3,
                                  TOKEN_SYMBOL_ASTERISK, TOKEN_SYMBOL_SLASH, TOKEN_SYMBOL_PERCENT)

DEFINE_BINARY_EXPR_PARSE_FUNCTION(parse_additive_expr, parse_multiplicative_expr, 2,
                                  TOKEN_SYMBOL_PLUS, TOKEN_SYMBOL_MINUS)

DEFINE_BINARY_EXPR_PARSE_FUNCTION(parse_comparison_expr, parse_additive_expr, 4,
                                  TOKEN_SYMBOL_LT, TOKEN_SYMBOL_GT, TOKEN_SYMBOL_LE,
                                  TOKEN_SYMBOL_GE)

DEFINE_BINARY_EXPR_PARSE_FUNCTION(parse_equality_expr, parse_comparison_expr, 2,
                                  TOKEN_SYMBOL_EQEQ, TOKEN_SYMBOL_BANGEQ)

DEFINE_BINARY_EXPR_PARSE_FUNCTION(parse_logical_and_expr, parse_equality_expr, 1,
                                  TOKEN_SYMBOL_ANDAND)

DEFINE_BINARY_EXPR_PARSE_FUNCTION(parse_logical_or_expr, parse_logical_and_expr, 1,
                                  TOKEN_SYMBOL_OROR)

static ast_node_t *parse_assignment_expr(void)
{
  ast_node_t *root = NULL, **node_p = &root;

  while (1)
  {
    token_t *token_identifier = token_try_advancing(1, TOKEN_IDENTIFIER);
    if (token_identifier == NULL)
      break;

    if (token_try_advancing(1, TOKEN_SYMBOL_EQ) == NULL)
    {
      token_unget();
      break;
    }

    ast_node_t *identifier_node = ast_node_primitive_expr_create(token_identifier);
    *node_p = ast_node_binary_expr_create(AST_EXPR_ASSIGNMENT, identifier_node, NULL);
    node_p = &(DATA_BINARY_EXPR(*node_p)->right_operand);
  }

  *node_p = parse_logical_or_expr();
  return root;
}

/*
 * Statements
 */

// <statement_list> ::= "{" { <statement> } "}"

static ast_node_t *parse_stmt_list(void)
{
  token_t *token;
  token_advance_and_assert(1, TOKEN_SYMBOL_LBRACE);

  ast_node_t *node = ast_node_stmt_list_create();

  while ((token = token_peek_next())->type != TOKEN_SYMBOL_RBRACE)
  {
    switch (token->type)
    {
      case TOKEN_SYMBOL_SEMICOLON: // null statement
        token_advance();
        break;
      case TOKEN_KEYWORD_IF:
        ast_node_stmt_list_append_stmt(node, parse_if_stmt());
        break;
      case TOKEN_KEYWORD_WHILE:
        ast_node_stmt_list_append_stmt(node, parse_while_stmt());
        break;
      case TOKEN_KEYWORD_RETURN:
        ast_node_stmt_list_append_stmt(node, parse_return_stmt());
        break;
      case TOKEN_KEYWORD_VAR:
        ast_node_stmt_list_append_stmt(node, parse_variable_decl());
        break;
      default:
        ast_node_stmt_list_append_stmt(node, parse_expr_stmt());
        break;
    }
  }

  token_advance();

  return node;
}

static ast_node_t *parse_expr_stmt(void)
{
  ast_node_t *node_expr = parse_expr();
  token_advance_and_assert(1, TOKEN_SYMBOL_SEMICOLON);
  return ast_node_stmt_create(AST_STMT_EXPR, NULL, node_expr);
}

static ast_node_t *parse_if_stmt(void)
{
  ast_node_t *node_expr, *node_stmt_list;

  token_advance_and_assert(1, TOKEN_KEYWORD_IF);

  token_advance_and_assert(1, TOKEN_SYMBOL_LPAREN);
  node_expr = parse_expr();
  token_advance_and_assert(1, TOKEN_SYMBOL_RPAREN);

  token_t *token = token_advance_and_assert(2, TOKEN_SYMBOL_LBRACE, TOKEN_SYMBOL_SEMICOLON);
  if (token->type == TOKEN_SYMBOL_LBRACE)
  {
    token_unget();
    node_stmt_list = parse_stmt_list();
  }
  else
    node_stmt_list = ast_node_stmt_list_create();

  return ast_node_stmt_create(AST_STMT_IF, node_stmt_list, node_expr);
}

static ast_node_t *parse_while_stmt(void)
{
  ast_node_t *node_expr, *node_stmt_list;

  token_advance_and_assert(1, TOKEN_KEYWORD_WHILE);

  token_advance_and_assert(1, TOKEN_SYMBOL_LPAREN);
  node_expr = parse_expr();
  token_advance_and_assert(1, TOKEN_SYMBOL_RPAREN);

  token_t *token = token_advance_and_assert(2, TOKEN_SYMBOL_LBRACE, TOKEN_SYMBOL_SEMICOLON);
  if (token->type == TOKEN_SYMBOL_LBRACE)
  {
    token_unget();
    node_stmt_list = parse_stmt_list();
  }
  else
    node_stmt_list = ast_node_stmt_list_create();

  return ast_node_stmt_create(AST_STMT_WHILE, node_stmt_list, node_expr);
}

static ast_node_t *parse_return_stmt(void)
{
  token_advance_and_assert(1, TOKEN_KEYWORD_RETURN);
  ast_node_t *node_expr = parse_expr();
  token_advance_and_assert(1, TOKEN_SYMBOL_SEMICOLON);

  return ast_node_stmt_create(AST_STMT_RETURN, NULL, node_expr);
}

/*
 * Declarations
 */

static data_type_t parse_data_type(void)
{
  token_t *token = token_advance_and_assert(1, TOKEN_KEYWORD_INT);

  switch (token->type)
  {
    case TOKEN_KEYWORD_INT:
      return DATA_TYPE_INT;
    default:
      __builtin_unreachable();
  }
}

static ast_node_t *parse_variable_decl(void)
{
  token_advance_and_assert(1, TOKEN_KEYWORD_VAR);
  token_t *token_identifier = token_advance_and_assert(1, TOKEN_IDENTIFIER);
  token_advance_and_assert(1, TOKEN_SYMBOL_COLON);
  data_type_t data_type = parse_data_type();

  ast_node_t *node_initializer = NULL;

  if (token_advance_and_assert(2, TOKEN_SYMBOL_SEMICOLON, TOKEN_SYMBOL_EQ)->type
      == TOKEN_SYMBOL_EQ)
  {
    node_initializer = parse_expr();
    token_advance_and_assert(1, TOKEN_SYMBOL_SEMICOLON);
  }

  return ast_node_variable_decl_create(token_identifier, data_type, node_initializer);
}

static ast_node_t *parse_function_parameter(void)
{
  token_advance_and_assert(1, TOKEN_KEYWORD_VAR);
  token_t *token_identifier = token_advance_and_assert(1, TOKEN_IDENTIFIER);
  token_advance_and_assert(1, TOKEN_SYMBOL_COLON);
  data_type_t data_type = parse_data_type();

  return ast_node_variable_decl_create(token_identifier, data_type, NULL);
}

static ast_node_t *parse_function_decl(void)
{
  token_advance_and_assert(1, TOKEN_KEYWORD_FN);
  token_t *token_identifier = token_advance_and_assert(1, TOKEN_IDENTIFIER);
  token_advance_and_assert(1, TOKEN_SYMBOL_LPAREN);

  ast_node_t *node = ast_node_function_decl_create(token_identifier);

  if (token_try_advancing(1, TOKEN_SYMBOL_RPAREN) == NULL)
  {
    do
      ast_node_function_decl_append_parameter(node, parse_function_parameter());
    while (token_advance_and_assert(2, TOKEN_SYMBOL_COMMA, TOKEN_SYMBOL_RPAREN)->type
           == TOKEN_SYMBOL_COMMA);
  }

  token_advance_and_assert(1, TOKEN_SYMBOL_COLON);

  DATA_FUNCTION_DECL(node)->return_data_type = parse_data_type();
  DATA_FUNCTION_DECL(node)->body = parse_stmt_list();

  return node;
}

/*
 * Program
 */

ast_node_t *parse(void)
{
  ast_node_t *node = ast_node_program_create();
  token_t *token;

  while ((token = token_peek_next()) != NULL)
  {
    switch (token->type)
    {
      case TOKEN_KEYWORD_VAR:
        ast_node_program_append_decl(node, parse_variable_decl());
        break;
      case TOKEN_KEYWORD_FN:
        ast_node_program_append_decl(node, parse_function_decl());
        break;
      default:
        exit_with_error("unexpected token\n");
        break;
    }
  }

  return node;
}
