#include <stdarg.h>
#include "yiche.h"

typedef struct parser_context_t
{
  tokenizer_context_t *tokenizer;
  token_t *last_unexpected_token;
}
parser_context_t;

parser_context_t *parser_context_create(tokenizer_context_t *tokenizer)
{
  parser_context_t *ctx = malloc(sizeof(parser_context_t));
  if (ctx == NULL)
    return NULL;

  ctx->tokenizer = tokenizer;
  return ctx;
}

void parser_context_free(parser_context_t *ctx)
{
  free(ctx);
}

void parser_print_error(parser_context_t *ctx, parser_result_t result)
{
  token_t *token;

  switch (result)
  {
    case PARSER_SUCCESS:
      return;
    case PARSER_OOM:
      fprintf(stdout, "out of memory");
      break;
    case PARSER_OUT_OF_TOKENS:
      fprintf(stdout, "syntax error: expected a token, got nothing\n");
      break;
    case PARSER_UNEXPECTED_TOKEN:
      token = ctx->last_unexpected_token;
      if (token == NULL)
        return;

      fprintf(stdout, "syntax error at line %d: unexpected token '%s'\n",
              token->char_begin.line, token->lexeme);
      break;
  }
}

#define FAIL(r) do \
  { \
    result = r; \
    goto fail; \
  } \
  while (0)

// Error propagation helper
#define PASSERT(expr) do \
  { \
    result = (expr); \
    if (result != PARSER_SUCCESS) \
      goto fail; \
  } \
  while (0)

/*
 * Token helpers
 */

// This function is used to define the functions below and should not be used
// directly.

static parser_result_t token_checkn(int n, token_t *token, ...)
{
  va_list types;
  if (token == NULL)
    return PARSER_OUT_OF_TOKENS;

  for (va_start(types, token); n--;)
  {
    if (token->type == va_arg(types, token_type_t))
    {
      va_end(types);
      return PARSER_SUCCESS;
    }
  }

  va_end(types);
  return PARSER_UNEXPECTED_TOKEN;
}

// token_check*(): If 'token' is 'NULL' or does not match any of the types, set
// 'ctx->last_unexpected_token' to 'token', and return the corresponding parser
// result code. Otherwise, return 'PARSER_SUCCESS'.

// token_checkp*(): Like above, but do not set the member in the context, and
// return booleans ('1' or '0'), for convenience.

static parser_result_t token_check(parser_context_t *ctx, token_t *token, token_type_t t1)
{
  parser_result_t result = token_checkn(1, token, t1);
  if (result != PARSER_SUCCESS)
    ctx->last_unexpected_token = token;
  return result;
}

static int token_checkp(token_t *token, token_type_t t1)
{
  return token_checkn(1, token, t1) == PARSER_SUCCESS;
}

static parser_result_t token_check2(parser_context_t *ctx, token_t *token, token_type_t t1,
                                    token_type_t t2)
{
  parser_result_t result = token_checkn(2, token, t1, t2);
  if (result != PARSER_SUCCESS)
    ctx->last_unexpected_token = token;
  return result;
}

static int token_checkp2(token_t *token, token_type_t t1, token_type_t t2)
{
  return token_checkn(2, token, t1, t2) == PARSER_SUCCESS;
}

static parser_result_t token_check3(parser_context_t *ctx, token_t *token, token_type_t t1,
                                    token_type_t t2, token_type_t t3)
{
  parser_result_t result = token_checkn(3, token, t1, t2, t3);
  if (result != PARSER_SUCCESS)
    ctx->last_unexpected_token = token;
  return result;
}

static int token_checkp3(token_t *token, token_type_t t1, token_type_t t2, token_type_t t3)
{
  return token_checkn(3, token, t1, t2, t3) == PARSER_SUCCESS;
}

static parser_result_t token_check4(parser_context_t *ctx, token_t *token, token_type_t t1,
                                    token_type_t t2, token_type_t t3, token_type_t t4)
{
  parser_result_t result = token_checkn(4, token, t1, t2, t3, t4);
  if (result != PARSER_SUCCESS)
    ctx->last_unexpected_token = token;
  return result;
}

static int token_checkp4(token_t *token, token_type_t t1, token_type_t t2, token_type_t t3,
                         token_type_t t4)
{
  return token_checkn(4, token, t1, t2, t3, t4) == PARSER_SUCCESS;
}

/*
 * Writing a parser function for a new binary operator:
 * 1. Add a function declaration below
 * 2. Add new mapping in 'get_binary_operator_type'
 * 3. Implement the function by either using a macro or manually writing it
 */

// Input: '*node_p' must be NULL.
//
// On success, '*node_p' is set to a valid pointer to the node, and
// 'PARSER_SUCCESS' is returned. The node can be used publicly (see ast.h). On
// failure, '*node_p' is NULL, and an error result code is returned.
static parser_result_t parse_expr(parser_context_t *ctx, ast_node_t **node_p);
static parser_result_t parse_primitive_expr(parser_context_t *ctx, ast_node_t **node_p);
static parser_result_t parse_function_call_expr(parser_context_t *ctx, ast_node_t **node_p);
static parser_result_t parse_prefix_expr(parser_context_t *ctx, ast_node_t **node_p);
static parser_result_t parse_multiplicative_expr(parser_context_t *ctx, ast_node_t **node_p);
static parser_result_t parse_additive_expr(parser_context_t *ctx, ast_node_t **node_p);
static parser_result_t parse_comparison_expr(parser_context_t *ctx, ast_node_t **node_p);
static parser_result_t parse_equality_expr(parser_context_t *ctx, ast_node_t **node_p);
static parser_result_t parse_logical_and_expr(parser_context_t *ctx, ast_node_t **node_p);
static parser_result_t parse_logical_or_expr(parser_context_t *ctx, ast_node_t **node_p);
static parser_result_t parse_assignment_expr(parser_context_t *ctx, ast_node_t **node_p);

static parser_result_t parse_stmt_list(parser_context_t *ctx, ast_node_t **node_p);
static parser_result_t parse_expr_stmt(parser_context_t *ctx, ast_node_t **node_p);
static parser_result_t parse_if_stmt(parser_context_t *ctx, ast_node_t **node_p);
static parser_result_t parse_while_stmt(parser_context_t *ctx, ast_node_t **node_p);
static parser_result_t parse_return_stmt(parser_context_t *ctx, ast_node_t **node_p);

static parser_result_t parse_variable_decl(parser_context_t *ctx, ast_node_t **node_p);
static parser_result_t parse_function_decl(parser_context_t *ctx, ast_node_t **node_p);

/*
 * Expressions
 */

static parser_result_t parse_expr(parser_context_t *ctx, ast_node_t **node_p)
{
  parser_result_t result;

  PASSERT(parse_assignment_expr(ctx, node_p));
  return PARSER_SUCCESS;

  fail:
    {
      if (*node_p != NULL)
      {
        ast_node_free(*node_p);
        *node_p = NULL;
      }
      return result;
    }
}

static parser_result_t parse_primitive_expr(parser_context_t *ctx, ast_node_t **node_p)
{
  parser_result_t result;

  token_t *token = tokenizer_consume(ctx->tokenizer);
  PASSERT(token_check3(ctx, token, TOKEN_IDENTIFIER, TOKEN_CONSTANT, TOKEN_SYMBOL_LPAREN));

  if (token->type == TOKEN_SYMBOL_LPAREN)
  {
    PASSERT(parse_expr(ctx, node_p));
    PASSERT(token_check(ctx, tokenizer_consume(ctx->tokenizer), TOKEN_SYMBOL_RPAREN));
    return PARSER_SUCCESS;
  }

  *node_p = ast_node_primitive_expr_create(token);
  if (*node_p == NULL)
    FAIL(PARSER_OOM);

  return PARSER_SUCCESS;

  fail:
    {
      if (*node_p != NULL)
      {
        ast_node_free(*node_p);
        *node_p = NULL;
      }
      return result;
    }
}

static parser_result_t parse_function_call_expr(parser_context_t *ctx, ast_node_t **node_p)
{
  parser_result_t result;
  ast_node_t *node_arg = NULL;

  token_t *token_callee = tokenizer_peek(ctx->tokenizer, 1), *next;
  if (!token_checkp(token_callee, TOKEN_IDENTIFIER) ||
      !token_checkp(tokenizer_peek(ctx->tokenizer, 2), TOKEN_SYMBOL_LPAREN))
  {
    PASSERT(parse_primitive_expr(ctx, node_p));
    return PARSER_SUCCESS;
  }

  tokenizer_consume(ctx->tokenizer);
  tokenizer_consume(ctx->tokenizer);

  *node_p = ast_node_function_call_expr_create(token_callee);
  if (*node_p == NULL)
    FAIL(PARSER_OOM);

  if ((next = tokenizer_peek(ctx->tokenizer, 1)) == NULL)
    FAIL(PARSER_OUT_OF_TOKENS);

  if (next->type == TOKEN_SYMBOL_RPAREN)
    tokenizer_consume(ctx->tokenizer);
  else
  {
    while (1)
    {
      PASSERT(parse_expr(ctx, &node_arg));

      if (!vector_append((*node_p)->data_function_call_expr.arguments, &node_arg))
        FAIL(PARSER_OOM);

      node_arg = NULL;

      token_t *token = tokenizer_consume(ctx->tokenizer);
      PASSERT(token_check2(ctx, token, TOKEN_SYMBOL_COMMA, TOKEN_SYMBOL_RPAREN));
      if (token->type != TOKEN_SYMBOL_COMMA)
        break;
    }
  }

  return PARSER_SUCCESS;

  fail:
    {
      if (*node_p != NULL)
      {
        ast_node_free(*node_p);
        *node_p = NULL;
      }
      if (node_arg != NULL)
        ast_node_free(node_arg);

      return result;
    }
}

static parser_result_t parse_prefix_expr(parser_context_t *ctx, ast_node_t **node_p)
{
  parser_result_t result;

  ast_node_t **current = node_p;
  *current = NULL;

  token_t *token_operator;

  while ((token_operator = tokenizer_peek(ctx->tokenizer, 1),
          token_checkp(token_operator, TOKEN_SYMBOL_BANG)))
  {
    tokenizer_consume(ctx->tokenizer);

    unary_operator_t operator;
    switch (token_operator->type)
    {
      case TOKEN_SYMBOL_BANG:
        operator = UNARY_OPERATOR_LOGICAL_NEGATION;
        break;
      default:
        break;
    }

    if ((*current = ast_node_unary_expr_create(operator, NULL)) == NULL)
      FAIL(PARSER_OOM);

    current = &((*current)->data_unary_expr.operand);
  }

  PASSERT(parse_function_call_expr(ctx, current));

  return PARSER_SUCCESS;

  fail:
    {
      if (*node_p != NULL)
      {
        ast_node_free(*node_p);
        *node_p = NULL;
      }
      return result;
    }
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
      exit_with_error("get_binary_operator_type(): missing binary operator mapping for symbol %d", symbol_type);
  }
}

// Left-recursive binary expressions are handled as right-recursive, e.g.,
// <multiplicative_expression> ::= <prefix_expression> [ ( "*" | "/" | "%" ) <multiplicative_expression> ]

#define DEFINE_BINARY_EXPR_PARSE_FUNCTION(func, prev, check_fn, ...) \
static parser_result_t func(parser_context_t *ctx, ast_node_t **node_p) \
{ \
  parser_result_t result; \
\
  ast_node_t *right_operand = NULL; \
\
  PASSERT(prev(ctx, node_p)); \
\
  token_t *token_operator; \
  while ((token_operator = tokenizer_peek(ctx->tokenizer, 1), \
          check_fn(token_operator, __VA_ARGS__))) \
  { \
    tokenizer_consume(ctx->tokenizer); \
    PASSERT(prev(ctx, &right_operand)); \
    binary_operator_t operator = get_binary_operator(token_operator->type); \
    ast_node_t *new = ast_node_binary_expr_create(operator, *node_p, right_operand); \
    if (new == NULL) \
      FAIL(PARSER_OOM); \
    right_operand = NULL; \
    *node_p = new; \
  } \
\
  return PARSER_SUCCESS; \
\
  fail: \
    { \
      if (*node_p != NULL) \
      { \
        ast_node_free(*node_p); \
        *node_p = NULL; \
      } \
\
      if (right_operand != NULL) \
        ast_node_free(right_operand); \
\
      return result; \
    } \
}

DEFINE_BINARY_EXPR_PARSE_FUNCTION(parse_multiplicative_expr, parse_prefix_expr, token_checkp3,
                                  TOKEN_SYMBOL_ASTERISK, TOKEN_SYMBOL_SLASH, TOKEN_SYMBOL_PERCENT)

DEFINE_BINARY_EXPR_PARSE_FUNCTION(parse_additive_expr, parse_multiplicative_expr, token_checkp2,
                                  TOKEN_SYMBOL_PLUS, TOKEN_SYMBOL_MINUS)

DEFINE_BINARY_EXPR_PARSE_FUNCTION(parse_comparison_expr, parse_additive_expr, token_checkp4,
                                  TOKEN_SYMBOL_LT, TOKEN_SYMBOL_GT, TOKEN_SYMBOL_LE,
                                  TOKEN_SYMBOL_GE)

DEFINE_BINARY_EXPR_PARSE_FUNCTION(parse_equality_expr, parse_comparison_expr, token_checkp2,
                                  TOKEN_SYMBOL_EQEQ, TOKEN_SYMBOL_BANGEQ)

DEFINE_BINARY_EXPR_PARSE_FUNCTION(parse_logical_and_expr, parse_equality_expr, token_checkp,
                                  TOKEN_SYMBOL_ANDAND)

DEFINE_BINARY_EXPR_PARSE_FUNCTION(parse_logical_or_expr, parse_logical_and_expr, token_checkp,
                                  TOKEN_SYMBOL_OROR)

static parser_result_t parse_assignment_expr(parser_context_t *ctx, ast_node_t **node_p)
{
  parser_result_t result;

  ast_node_t **current = node_p;
  *current = NULL;

  while (1)
  {
    token_t *token_identifier = tokenizer_peek(ctx->tokenizer, 1);
    if (!token_checkp(token_identifier, TOKEN_IDENTIFIER))
      break;

    if (!token_checkp(tokenizer_peek(ctx->tokenizer, 2), TOKEN_SYMBOL_EQ))
      break;

    tokenizer_consume(ctx->tokenizer);
    tokenizer_consume(ctx->tokenizer);

    ast_node_t *identifier_node = ast_node_primitive_expr_create(token_identifier);
    if (identifier_node == NULL)
      FAIL(PARSER_OOM);

    *current = ast_node_binary_expr_create(BINARY_OPERATOR_ASSIGNMENT, identifier_node, NULL);
    if (*current == NULL)
    {
      ast_node_free(identifier_node);
      FAIL(PARSER_OOM);
    }

    current = &((*current)->data_binary_expr.right_operand);
  }

  // invariant: *current == NULL
  PASSERT(parse_logical_or_expr(ctx, current));

  return PARSER_SUCCESS;

  fail:
    {
      if (*node_p != NULL)
      {
        ast_node_free(*node_p);
        *node_p = NULL;
      }
      return result;
    }
}

/*
 * Statements
 */

// <statement_list> ::= "{" { <statement> } "}"

static parser_result_t parse_stmt_list(parser_context_t *ctx, ast_node_t **node_p)
{
  parser_result_t result;

  ast_node_t *child = NULL;

  token_t *token;
  PASSERT(token_check(ctx, tokenizer_consume(ctx->tokenizer), TOKEN_SYMBOL_LBRACE));

  *node_p = ast_node_stmt_list_create();
  if (*node_p == NULL)
    FAIL(PARSER_OOM);

  while ((token = tokenizer_peek(ctx->tokenizer, 1)) != NULL && token->type != TOKEN_SYMBOL_RBRACE)
  {
    if (token->type == TOKEN_SYMBOL_SEMICOLON)
    {
      tokenizer_consume(ctx->tokenizer);
      continue;
    }

    switch (token->type)
    {
      case TOKEN_KEYWORD_IF:
        PASSERT(parse_if_stmt(ctx, &child));
        break;
      case TOKEN_KEYWORD_WHILE:
        PASSERT(parse_while_stmt(ctx, &child));
        break;
      case TOKEN_KEYWORD_RETURN:
        PASSERT(parse_return_stmt(ctx, &child));
        break;
      case TOKEN_KEYWORD_VAR:
        PASSERT(parse_variable_decl(ctx, &child));
        break;
      default:
        PASSERT(parse_expr_stmt(ctx, &child));
        break;
    }

    if (!vector_append((*node_p)->data_stmt_list.stmts, &child))
      FAIL(PARSER_OOM);

    child = NULL;
  }

  if (token == NULL)
    FAIL(PARSER_OUT_OF_TOKENS);

  tokenizer_consume(ctx->tokenizer);

  return PARSER_SUCCESS;

  fail:
    {
      if (*node_p != NULL)
      {
        ast_node_free(*node_p);
        *node_p = NULL;
      }

      if (child != NULL)
        ast_node_free(child);

      return result;
    }
}

static parser_result_t parse_expr_stmt(parser_context_t *ctx, ast_node_t **node_p)
{
  parser_result_t result;

  *node_p = ast_node_expr_stmt_create(NULL);
  if (*node_p == NULL)
    FAIL(PARSER_OOM);

  PASSERT(parse_expr(ctx, &(*node_p)->data_expr_stmt.expr));
  PASSERT(token_check(ctx, tokenizer_consume(ctx->tokenizer), TOKEN_SYMBOL_SEMICOLON));

  return PARSER_SUCCESS;

  fail:
    {
      if (*node_p != NULL)
      {
        ast_node_free(*node_p);
        *node_p = NULL;
      }

      return result;
    }
}

static parser_result_t parse_if_stmt(parser_context_t *ctx, ast_node_t **node_p)
{
  parser_result_t result;

  *node_p = ast_node_if_stmt_create(NULL, NULL);
  if (*node_p == NULL)
    FAIL(PARSER_OOM);

  PASSERT(token_check(ctx, tokenizer_consume(ctx->tokenizer), TOKEN_KEYWORD_IF));

  PASSERT(token_check(ctx, tokenizer_consume(ctx->tokenizer), TOKEN_SYMBOL_LPAREN));
  PASSERT(parse_expr(ctx, &(*node_p)->data_if_stmt.cond_expr));
  PASSERT(token_check(ctx, tokenizer_consume(ctx->tokenizer), TOKEN_SYMBOL_RPAREN));

  token_t *token = tokenizer_peek(ctx->tokenizer, 1);
  PASSERT(token_check2(ctx, token, TOKEN_SYMBOL_LBRACE, TOKEN_SYMBOL_SEMICOLON));
  if (token->type == TOKEN_SYMBOL_LBRACE)
  {
    PASSERT(parse_stmt_list(ctx, &(*node_p)->data_if_stmt.stmt_list));
  }
  else
  {
    tokenizer_consume(ctx->tokenizer);

    (*node_p)->data_if_stmt.stmt_list = ast_node_stmt_list_create();
    if ((*node_p)->data_if_stmt.stmt_list == NULL)
      FAIL(PARSER_OOM);
  }

  return PARSER_SUCCESS;

  fail:
    {
      if (*node_p != NULL)
      {
        ast_node_free(*node_p);
        *node_p = NULL;
      }

      return result;
    }
}

static parser_result_t parse_while_stmt(parser_context_t *ctx, ast_node_t **node_p)
{
  parser_result_t result;

  *node_p = ast_node_while_stmt_create(NULL, NULL);
  if (*node_p == NULL)
    FAIL(PARSER_OOM);

  PASSERT(token_check(ctx, tokenizer_consume(ctx->tokenizer), TOKEN_KEYWORD_WHILE));

  PASSERT(token_check(ctx, tokenizer_consume(ctx->tokenizer), TOKEN_SYMBOL_LPAREN));
  PASSERT(parse_expr(ctx, &(*node_p)->data_while_stmt.cond_expr));
  PASSERT(token_check(ctx, tokenizer_consume(ctx->tokenizer), TOKEN_SYMBOL_RPAREN));

  token_t *token = tokenizer_peek(ctx->tokenizer, 1);
  PASSERT(token_check2(ctx, token, TOKEN_SYMBOL_LBRACE, TOKEN_SYMBOL_SEMICOLON));

  if (token->type == TOKEN_SYMBOL_LBRACE)
  {
    PASSERT(parse_stmt_list(ctx, &(*node_p)->data_while_stmt.stmt_list));
  }
  else
  {
    tokenizer_consume(ctx->tokenizer);

    (*node_p)->data_while_stmt.stmt_list = ast_node_stmt_list_create();
    if ((*node_p)->data_while_stmt.stmt_list == NULL)
      FAIL(PARSER_OOM);
  }

  return PARSER_SUCCESS;

  fail:
    {
      if (*node_p != NULL)
      {
        ast_node_free(*node_p);
        *node_p = NULL;
      }

      return result;
    }
}

static parser_result_t parse_return_stmt(parser_context_t *ctx, ast_node_t **node_p)
{
  parser_result_t result;

  if ((*node_p = ast_node_return_stmt_create(NULL)) == NULL)
    FAIL(PARSER_OOM);

  PASSERT(token_check(ctx, tokenizer_consume(ctx->tokenizer), TOKEN_KEYWORD_RETURN));
  PASSERT(parse_expr(ctx, &(*node_p)->data_return_stmt.expr));
  PASSERT(token_check(ctx, tokenizer_consume(ctx->tokenizer), TOKEN_SYMBOL_SEMICOLON));

  return PARSER_SUCCESS;

  fail:
    {
      if (*node_p != NULL)
      {
        ast_node_free(*node_p);
        *node_p = NULL;
      }

      return result;
    }
}

/*
 * Declarations
 */

static parser_result_t parse_data_type(parser_context_t *ctx, data_type_t *data_type)
{
  parser_result_t result;

  token_t *token = tokenizer_consume(ctx->tokenizer);
  PASSERT(token_check(ctx, token, TOKEN_KEYWORD_INT));

  switch (token->type)
  {
    case TOKEN_KEYWORD_INT:
      *data_type = DATA_TYPE_INT;
      break;
    default:
      UNREACHABLE;
  }

  return PARSER_SUCCESS;

  fail:
    {
      return result;
    }
}

static parser_result_t parse_variable_decl(parser_context_t *ctx, ast_node_t **node_p)
{
  parser_result_t result;

  PASSERT(token_check(ctx, tokenizer_consume(ctx->tokenizer), TOKEN_KEYWORD_VAR));

  token_t *token_identifier = tokenizer_consume(ctx->tokenizer);
  PASSERT(token_check(ctx, token_identifier, TOKEN_IDENTIFIER));

  PASSERT(token_check(ctx, tokenizer_consume(ctx->tokenizer), TOKEN_SYMBOL_COLON));
  data_type_t data_type;
  PASSERT(parse_data_type(ctx, &data_type));

  *node_p = ast_node_variable_decl_create(token_identifier, data_type, NULL);
  if (*node_p == NULL)
    FAIL(PARSER_OOM);

  token_t *token = tokenizer_consume(ctx->tokenizer);
  PASSERT(token_check2(ctx, token, TOKEN_SYMBOL_SEMICOLON, TOKEN_SYMBOL_EQ));
  if (token->type == TOKEN_SYMBOL_EQ)
  {
    PASSERT(parse_expr(ctx, &(*node_p)->data_variable_decl.initializer));
    PASSERT(token_check(ctx, tokenizer_consume(ctx->tokenizer), TOKEN_SYMBOL_SEMICOLON));
  }

  return PARSER_SUCCESS;

  fail:
    {
      if (*node_p != NULL)
      {
        ast_node_free(*node_p);
        *node_p = NULL;
      }

      return result;
    }
}

static parser_result_t parse_function_parameter(parser_context_t *ctx, ast_node_t **node_p)
{
  parser_result_t result;

  PASSERT(token_check(ctx, tokenizer_consume(ctx->tokenizer), TOKEN_KEYWORD_VAR));

  token_t *token_identifier = tokenizer_consume(ctx->tokenizer);
  PASSERT(token_check(ctx, token_identifier, TOKEN_IDENTIFIER));

  PASSERT(token_check(ctx, tokenizer_consume(ctx->tokenizer), TOKEN_SYMBOL_COLON));
  data_type_t data_type;
  PASSERT(parse_data_type(ctx, &data_type));

  *node_p = ast_node_variable_decl_create(token_identifier, data_type, NULL);
  if (*node_p == NULL)
    FAIL(PARSER_OOM);

  return PARSER_SUCCESS;

  fail:
    {
      if (*node_p != NULL)
      {
        ast_node_free(*node_p);
        *node_p = NULL;
      }

      return result;
    }
}

static parser_result_t parse_function_decl(parser_context_t *ctx, ast_node_t **node_p)
{
  parser_result_t result;
  ast_node_t *node_param = NULL;

  PASSERT(token_check(ctx, tokenizer_consume(ctx->tokenizer), TOKEN_KEYWORD_FN));

  token_t *token_identifier = tokenizer_consume(ctx->tokenizer);
  PASSERT(token_check(ctx, token_identifier, TOKEN_IDENTIFIER));
  PASSERT(token_check(ctx, tokenizer_consume(ctx->tokenizer), TOKEN_SYMBOL_LPAREN));

  *node_p = ast_node_function_decl_create(token_identifier);
  if (*node_p == NULL)
    FAIL(PARSER_OOM);

  if (!token_checkp(tokenizer_peek(ctx->tokenizer, 1), TOKEN_SYMBOL_RPAREN))
  {
    while (1)
    {
      PASSERT(parse_function_parameter(ctx, &node_param));
      if (!vector_append((*node_p)->data_function_decl.parameters, &node_param))
        FAIL(PARSER_OOM);
      node_param = NULL;

      token_t *token = tokenizer_consume(ctx->tokenizer);
      PASSERT(token_check2(ctx, token, TOKEN_SYMBOL_COMMA, TOKEN_SYMBOL_RPAREN));
      if (token->type != TOKEN_SYMBOL_COMMA)
        break;
    }
  }
  else
    tokenizer_consume(ctx->tokenizer);

  PASSERT(token_check(ctx, tokenizer_consume(ctx->tokenizer), TOKEN_SYMBOL_COLON));

  PASSERT(parse_data_type(ctx, &(*node_p)->data_function_decl.return_data_type));
  PASSERT(parse_stmt_list(ctx, &(*node_p)->data_function_decl.body));

  return PARSER_SUCCESS;

  fail:
    {
      if (*node_p != NULL)
      {
        ast_node_free(*node_p);
        *node_p = NULL;
      }

      if (node_param != NULL)
        ast_node_free(node_param);

      return result;
    }
}

/*
 * Program
 */

parser_result_t parse_program(parser_context_t *ctx, ast_node_t **node_p)
{
  parser_result_t result;

  ast_node_t *node_decl = NULL;

  *node_p = ast_node_program_create();
  if (*node_p == NULL)
    FAIL(PARSER_OOM);

  token_t *token;

  while ((token = tokenizer_peek(ctx->tokenizer, 1)) != NULL)
  {
    switch (token->type)
    {
      case TOKEN_KEYWORD_VAR:
        PASSERT(parse_variable_decl(ctx, &node_decl));
        break;
      case TOKEN_KEYWORD_FN:
        PASSERT(parse_function_decl(ctx, &node_decl));
        break;
      default:
      {
        ctx->last_unexpected_token = token;
        FAIL(PARSER_UNEXPECTED_TOKEN);
      }
    }

    if (!vector_append((*node_p)->data_program.decls, &node_decl))
      FAIL(PARSER_OOM);

    node_decl = NULL;
  }

  return PARSER_SUCCESS;

  fail:
    {
      if (*node_p != NULL)
      {
        ast_node_free(*node_p);
        *node_p = NULL;
      }

      if (node_decl != NULL)
        ast_node_free(node_decl);

      return result;
    }
}

parser_result_t parser_parse(parser_context_t *ctx, ast_node_t **node_p)
{
  parser_result_t result;

  result = parse_program(ctx, node_p);
  if (result != PARSER_SUCCESS)
  {
    ast_node_free(*node_p);
    *node_p = NULL;
  }

  return result;
}
