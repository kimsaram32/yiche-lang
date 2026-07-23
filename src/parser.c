#include "yiche.h"

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

static ast_node_t *parse_function_call_expr(void)
{
  token_t *token_callee = token_try_advancing(1, TOKEN_IDENTIFIER);
  if (token_callee == NULL)
    return parse_primitive_expr();

  if (token_try_advancing(1, TOKEN_SYMBOL_LPAREN) == NULL)
  {
    token_unget();
    return parse_primitive_expr();
  }

  ast_node_t *node = ast_node_function_call_expr_create(token_callee);

  if (token_peek_next()->type == TOKEN_SYMBOL_RPAREN)
    token_advance();
  else
  {
    do
      ast_node_function_call_expr_append_argument(node, parse_expr());
    while (token_advance_and_assert(2, TOKEN_SYMBOL_COMMA, TOKEN_SYMBOL_RPAREN)->type
           == TOKEN_SYMBOL_COMMA);
  }

  return node;
}

// Switch to below when function types are supported.

/* // <function_call_expression> ::= <primitive_expression> { "(" [ <function_call_argument_list> ] ")" } */

/* static ast_node_t *parse_function_call_expr(void) */
/* { */
/*   ast_node_t *root = parse_primitive_expr(); */

/*   while (token_try_advancing(1, TOKEN_SYMBOL_LPAREN)) */
/*   { */
/*     root = ast_node_function_call_expr_create(root); */

/*     if (token_try_advancing(1, TOKEN_SYMBOL_RPAREN) == NULL) */
/*     { */
/*       do */
/*         ast_node_function_call_expr_append_argument(root, parse_expr()); */
/*       while (token_advance_and_assert(2, TOKEN_SYMBOL_COMMA, TOKEN_SYMBOL_RPAREN)->type */
/*              == TOKEN_SYMBOL_COMMA); */
/*     } */
/*   } */

/*   return root; */
/* } */

static ast_node_t *parse_prefix_expr(void)
{
  ast_node_t *root = NULL, **node_p = &root;

  token_t *token_operator;

  while ((token_operator = token_try_advancing(1, TOKEN_SYMBOL_BANG)) != NULL)
  {
    unary_operator_t operator;
    switch (token_operator->type)
    {
      case TOKEN_SYMBOL_BANG:
        operator = UNARY_OPERATOR_LOGICAL_NEGATION;
        break;
      default:
        break;
    }
    *node_p = ast_node_unary_expr_create(operator, NULL);
    node_p = &((*node_p)->data_unary_expr.operand);
  }

  *node_p = parse_function_call_expr();
  return root;
}

static binary_operator_t get_binary_operator(token_type_t symbol_type)
{
  switch (symbol_type)
  {
    case TOKEN_SYMBOL_ASTERISK:
      return BINARY_OPERATOR_MULTIPLICATION;
    case TOKEN_SYMBOL_SLASH:
      return BINARY_OPERATOR_DIVISION;
    case TOKEN_SYMBOL_PERCENT:
      return BINARY_OPERATOR_MODULO;

    case TOKEN_SYMBOL_PLUS:
      return BINARY_OPERATOR_ADDITION;
    case TOKEN_SYMBOL_MINUS:
      return BINARY_OPERATOR_SUBTRACTION;

    case TOKEN_SYMBOL_LT:
      return BINARY_OPERATOR_LESS_THAN;
    case TOKEN_SYMBOL_GT:
      return BINARY_OPERATOR_GREATER_THAN;
    case TOKEN_SYMBOL_LE:
      return BINARY_OPERATOR_LESS_THAN_EQUAL_TO;
    case TOKEN_SYMBOL_GE:
      return BINARY_OPERATOR_GREATER_THAN_EQUAL_TO;

    case TOKEN_SYMBOL_EQEQ:
      return BINARY_OPERATOR_EQUALS;
    case TOKEN_SYMBOL_BANGEQ:
      return BINARY_OPERATOR_NOT_EQUALS;

    case TOKEN_SYMBOL_ANDAND:
      return BINARY_OPERATOR_LOGICAL_AND;

    case TOKEN_SYMBOL_OROR:
      return BINARY_OPERATOR_LOGICAL_OR;

    case TOKEN_SYMBOL_EQ:
      return BINARY_OPERATOR_ASSIGNMENT;

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
    binary_operator_t operator = get_binary_operator(token_operator->type); \
    root = ast_node_binary_expr_create(operator, root, right_operand); \
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
    *node_p = ast_node_binary_expr_create(BINARY_OPERATOR_ASSIGNMENT, identifier_node, NULL);
    node_p = &((*node_p)->data_binary_expr.right_operand);
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
  return ast_node_expr_stmt_create(node_expr);
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

  return ast_node_if_stmt_create(node_expr, node_stmt_list);
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

  return ast_node_while_stmt_create(node_expr, node_stmt_list);
}

static ast_node_t *parse_return_stmt(void)
{
  token_advance_and_assert(1, TOKEN_KEYWORD_RETURN);
  ast_node_t *node_expr = parse_expr();
  token_advance_and_assert(1, TOKEN_SYMBOL_SEMICOLON);

  return ast_node_return_stmt_create(node_expr);
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
      UNREACHABLE;
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

  node->data_function_decl.return_data_type = parse_data_type();
  node->data_function_decl.body = parse_stmt_list();

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
