#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "yiche.h"

/*
 * Result
 */

static eval_value_t value_int(int v)
{
  return (eval_value_t) {
    .data_type = DATA_TYPE_INT,
    .value_int = v,
  };
}

// The success payload differs by function category.
// - expressions: set 'value'.
// - statements: set 'returned'. If 'returned' is '1', also set 'value'.
// - declarations and other internal functions: nothing, unless stated otherwise.
typedef struct
{
  eval_result_type_t type;
  union
  {
    struct
    {
      eval_value_t value;
      int returned;
    };
    char *error_message;
  };
}
eval_result_internal_t;

// Helpers

static eval_result_internal_t RESULT_SUCCESS = {
  .type = EVAL_SUCCESS,
};

static eval_result_internal_t result_success_expr(eval_value_t value)
{
  return (eval_result_internal_t) {
    .type = EVAL_SUCCESS,
    .value = value,
  };
}

static eval_result_internal_t RESULT_SUCCESS_STMT_NO_RETURN = {
  .type = EVAL_SUCCESS,
  .returned = 0,
};

static eval_result_internal_t RESULT_OOM = {
  .type = EVAL_OOM,
  .error_message = "out of memory",
};

static eval_result_internal_t _result_runtime_error(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  int length = vsnprintf(NULL, 0, format, args);
  va_end(args);

  va_start(args, format);
  char *buf = malloc(sizeof(char) * (length + 1));
  if (buf == NULL)
    return RESULT_OOM;

  vsnprintf(buf, length + 1, format, args);
  va_end(args);

  return (eval_result_internal_t) {
    .type = EVAL_RUNTIME_ERROR,
    .error_message = buf,
  };
}

#define RESULT_RUNTIME_ERROR(...) _result_runtime_error("runtime error: " __VA_ARGS__)

/*
 * Types
 */

typedef struct eval_env_t
{
  HASH_TABLE_T(symbol_table_entry_t) *symbol_table;
  struct eval_env_t *parent, *prev;
}
eval_env_t;

typedef struct
{
  hash_table_t *builtin_symbol_table;
  eval_env_t *current_env, *global_env;
}
eval_context_t;

typedef enum
{
  SYMBOL_TYPE_VARIABLE,
  SYMBOL_TYPE_FUNCTION,
}
symbol_type_t;

typedef struct
{
  eval_value_t value;
}
symbol_table_entry_variable_t;

typedef enum
{
  FUNCTION_TYPE_RUNTIME,
  FUNCTION_TYPE_BUILTIN,
}
function_type_t;

// Invariant: accesses to 'args[i]' with '0 <= i < builtin_params_length' are valid.
// The output contract is the same as that of expression evaluators.
typedef eval_result_internal_t (*builtin_function_t)(eval_context_t *ctx, eval_value_t *args);

typedef struct
{
  function_type_t type;
  ast_node_t *runtime_node;
  int builtin_parameters_length;
  builtin_function_t builtin_function;
}
symbol_table_entry_function_t;

typedef struct
{
  char *symbol_identifier;
  symbol_type_t symbol_type;
  union
  {
    symbol_table_entry_variable_t data_variable;
    symbol_table_entry_function_t data_function;
  };
}
symbol_table_entry_t;

/*
 * Symbol tables
 */

static void symbol_table_entry_free(void *p)
{
  symbol_table_entry_t *entry = p;
  free(entry);
}

#define SYMBOL_TABLE_INITIAL_CAPACITY 107

static hash_table_t *symbol_table_create(void)
{
  return hash_table_create(SYMBOL_TABLE_INITIAL_CAPACITY,
                           symbol_table_entry_free);
}

/*
 * Execution environments
 */

// The ownership to prev (but not parent) is transferred to the new environment.
static eval_env_t *eval_env_create(eval_env_t *parent, eval_env_t *prev)
{
  eval_env_t *env = malloc(sizeof(eval_env_t));
  if (env == NULL)
    return NULL;

  env->symbol_table = symbol_table_create();
  if (env->symbol_table == NULL)
  {
    free(env);
    return NULL;
  }

  env->parent = parent;
  env->prev = prev;

  return env;
}

// Because the previous environment (env->prev) is not freed, its ownership
// should be transferred before calling the function.
static void eval_env_free(eval_env_t *env)
{
  hash_table_free(env->symbol_table);
  free(env);
}

static eval_result_internal_t eval_env_add_symbol_table_entry(eval_env_t *env, symbol_table_entry_t *entry)
{
  if (hash_table_search(env->symbol_table, entry->symbol_identifier))
    return RESULT_RUNTIME_ERROR("identifier '%s' already declared in the current scope", entry->symbol_identifier);

  if (!hash_table_insert(env->symbol_table, entry->symbol_identifier, entry))
    return RESULT_OOM;

  return RESULT_SUCCESS;
}

/*
 * Built-in functions
 */

static eval_result_internal_t builtin_function_print(eval_context_t *ctx,
                                                     eval_value_t *arguments)
{
  eval_value_t n = arguments[0];

  printf("%d", n.value_int);

  return result_success_expr(value_int(0));
}

static eval_result_internal_t builtin_function_println(eval_context_t *ctx,
                                                       eval_value_t *arguments)
{
  eval_value_t n = arguments[0];

  printf("%d\n", n.value_int);

  return result_success_expr(value_int(0));
}

static eval_result_internal_t builtin_function_print_char(eval_context_t *ctx,
                                                          eval_value_t *arguments)
{
  eval_value_t n = arguments[0];

  printf("%c", n.value_int);

  return result_success_expr(value_int(0));
}

#define BUILTIN_FUNCTION_SYMBOL_TABLE_ENTRY(name, parameters_length) \
  { \
    .symbol_identifier = # name, \
    .symbol_type = SYMBOL_TYPE_FUNCTION, \
    .data_function = { \
      .type = FUNCTION_TYPE_BUILTIN, \
      .builtin_parameters_length = parameters_length, \
      .builtin_function = builtin_function_ ## name, \
    }, \
  }

static symbol_table_entry_t builtin_symbol_table_entries[] = {
  BUILTIN_FUNCTION_SYMBOL_TABLE_ENTRY(print, 1),
  BUILTIN_FUNCTION_SYMBOL_TABLE_ENTRY(println, 1),
  BUILTIN_FUNCTION_SYMBOL_TABLE_ENTRY(print_char, 1),
};

// 'table' should not contain any elements with the same name as one of the
// built-in functions.
//
// On out-of-memory error, return '0'. Entries may be partially inserted.
static int builtin_add_symbol_table_entries(hash_table_t *table)
{
  for (int i = 0; i < ARRAY_LENGTH(builtin_symbol_table_entries); i++)
  {
    symbol_table_entry_t *entry = malloc(sizeof(symbol_table_entry_t));
    if (entry == NULL)
      return 0;

    memcpy(entry, builtin_symbol_table_entries + i, sizeof(symbol_table_entry_t));

    if (!hash_table_insert(table, entry->symbol_identifier, entry))
      return 0;
  }

  return 1;
}

/*
 * Evaluation context
 */

static eval_context_t *eval_context_create(void)
{
  eval_context_t *ctx = malloc(sizeof(eval_context_t));
  if (ctx == NULL)
    return NULL;

  if ((ctx->builtin_symbol_table = symbol_table_create()) == NULL)
  {
    free(ctx);
    return NULL;
  }

  if ((ctx->global_env = eval_env_create(NULL, NULL)) == NULL)
  {
    hash_table_free(ctx->builtin_symbol_table);
    free(ctx);
    return NULL;
  }

  ctx->current_env = ctx->global_env;

  if (!builtin_add_symbol_table_entries(ctx->builtin_symbol_table))
  {
    eval_env_free(ctx->global_env);
    hash_table_free(ctx->builtin_symbol_table);
    free(ctx);
    return NULL;
  }

  return ctx;
}

static void eval_context_free(eval_context_t *ctx)
{
  eval_env_t *env = ctx->current_env, *next_env;

  hash_table_free(ctx->builtin_symbol_table);

  while (env != NULL)
  {
    next_env = env->prev;
    eval_env_free(env);
    env = next_env;
  }

  free(ctx);
}

// Returns '0' if an out-of-memory error occurs.
static int eval_context_enter_new_env(eval_context_t *ctx, eval_env_t *parent)
{
  eval_env_t *new_env = eval_env_create(parent, ctx->current_env);
  if (new_env == NULL)
    return 0;

  ctx->current_env = new_env;
  return 1;
}

static void eval_context_exit_current_env(eval_context_t *ctx)
{
  if (ctx->current_env == ctx->global_env)
    exit_with_error("eval_context_exit_current_env(): cannot exit the global environment");

  eval_env_t *prev_env = ctx->current_env->prev;
  eval_env_free(ctx->current_env);
  ctx->current_env = prev_env;
}

static symbol_table_entry_t *eval_context_lookup_symbol_table_entry(eval_context_t *ctx,
                                                                    char *symbol_identifier)
{
  eval_env_t *env = ctx->current_env;
  while (env != NULL)
  {
    symbol_table_entry_t *entry = hash_table_search(env->symbol_table, symbol_identifier);
    if (entry != NULL)
      return entry;
    env = env->parent;
  }
  return hash_table_search(ctx->builtin_symbol_table, symbol_identifier);
}

static eval_result_internal_t eval_context_lookup_variable(eval_context_t *ctx,
                                                           char *symbol_identifier,
                                                           symbol_table_entry_t **entry_p)
{
  *entry_p = eval_context_lookup_symbol_table_entry(ctx, symbol_identifier);

  if (*entry_p == NULL)
    return RESULT_RUNTIME_ERROR("undeclared identifier '%s'", symbol_identifier);
  else if ((*entry_p)->symbol_type != SYMBOL_TYPE_VARIABLE)
    return RESULT_RUNTIME_ERROR("identifier '%s' is not a variable", symbol_identifier);

  return RESULT_SUCCESS;
}

static eval_result_internal_t eval_context_lookup_function(eval_context_t *ctx,
                                                            char *symbol_identifier,
                                                            symbol_table_entry_t **entry_p)
{
  *entry_p = eval_context_lookup_symbol_table_entry(ctx, symbol_identifier);

  if (*entry_p == NULL)
    return RESULT_RUNTIME_ERROR("undeclared identifier '%s'", symbol_identifier);
  else if ((*entry_p)->symbol_type != SYMBOL_TYPE_FUNCTION)
    return RESULT_RUNTIME_ERROR("identifier '%s' is not a function", symbol_identifier);

  return RESULT_SUCCESS;
}

/*
 * Evaluation
 */

static eval_result_internal_t eval_primitive_expr(eval_context_t *ctx, ast_node_t *node);
static eval_result_internal_t eval_unary_expr(eval_context_t *ctx, ast_node_t *node);
static eval_result_internal_t eval_binary_expr(eval_context_t *ctx, ast_node_t *node);
static eval_result_internal_t eval_function_call_expr(eval_context_t *ctx, ast_node_t *node);

static eval_result_internal_t eval_expr_stmt(eval_context_t *ctx, ast_node_t *node);
static eval_result_internal_t eval_if_stmt(eval_context_t *ctx, ast_node_t *node);
static eval_result_internal_t eval_while_stmt(eval_context_t *ctx, ast_node_t *node);
static eval_result_internal_t eval_return_stmt(eval_context_t *ctx, ast_node_t *node);
static eval_result_internal_t eval_stmt_list(eval_context_t *ctx, ast_node_t *node);

static eval_result_internal_t eval_variable_decl(eval_context_t *ctx, ast_node_t *node);
static eval_result_internal_t eval_function_decl(eval_context_t *ctx, ast_node_t *node);

static eval_result_internal_t result_runtime_error_argument_mismatch(int expected, int passed,
                                                                     char *identifier)
{
  return RESULT_RUNTIME_ERROR("function call argument mismatch: '%s' expects %d argument(s),"
                      " but %d got passed", identifier, expected, passed);
}

// These also apply to 'call_runtime_function' and 'call_function' below.
//
// 'arguments + i' must be a valid reference to 'eval_value_t' for all 0
// <= i < arguments_length.
//
// The output contract is the same as that of expression evaluators.
static eval_result_internal_t call_builtin_function(eval_context_t *ctx,
                                                    symbol_table_entry_t *entry,
                                                    eval_value_t *arguments, int arguments_length)
{
  if (entry->data_function.builtin_parameters_length != arguments_length)
    return result_runtime_error_argument_mismatch(entry->data_function.builtin_parameters_length,
                                                  arguments_length,
                                                  entry->symbol_identifier);

  return entry->data_function.builtin_function(ctx, arguments);
}

static eval_result_internal_t call_runtime_function(eval_context_t *ctx, ast_node_t *runtime_node,
                                                    eval_value_t *arguments, int arguments_length)
{
  ast_node_function_decl_t function_data = runtime_node->data_function_decl;
  ast_node_t **parameters_arr = VECTOR_ARR(function_data.parameters, ast_node_t*);

  if (function_data.parameters->length != arguments_length)
    return result_runtime_error_argument_mismatch(function_data.parameters->length,
                                                  arguments_length,
                                                  function_data.token_identifier->identifier);

  if (!eval_context_enter_new_env(ctx, ctx->global_env))
    return RESULT_OOM;

  for (int i = 0; i < arguments_length; i++)
  {
    char *param_identifier = parameters_arr[i]->data_variable_decl.token_identifier->identifier;
    // TODO: check data types when proper support is added

    symbol_table_entry_t *symbol_table_entry = malloc(sizeof(symbol_table_entry_t));
    if (symbol_table_entry == NULL)
    {
      eval_context_exit_current_env(ctx);
      return RESULT_OOM;
    }

    symbol_table_entry->symbol_identifier = param_identifier;
    symbol_table_entry->symbol_type = SYMBOL_TYPE_VARIABLE;
    symbol_table_entry->data_variable.value = arguments[i];

    eval_result_internal_t add_result =
      eval_env_add_symbol_table_entry(ctx->current_env, symbol_table_entry);
    if (add_result.type != EVAL_SUCCESS)
    {
      symbol_table_entry_free(symbol_table_entry);
      eval_context_exit_current_env(ctx);
      return add_result;
    }
  }

  eval_result_internal_t stmt_result = eval_stmt_list(ctx, runtime_node->data_function_decl.body);

  eval_context_exit_current_env(ctx);

  if (stmt_result.type != EVAL_SUCCESS)
    return stmt_result;

  if (!stmt_result.returned)
    return RESULT_RUNTIME_ERROR("function '%s' does not return a value",
                                runtime_node->data_function_decl.token_identifier->identifier);

  return stmt_result;
}

static eval_result_internal_t call_function(eval_context_t *ctx, symbol_table_entry_t *entry,
                                            eval_value_t *arguments, int arguments_length)
{
  switch (entry->data_function.type)
  {
    case FUNCTION_TYPE_BUILTIN:
      return call_builtin_function(ctx, entry, arguments, arguments_length);
    case FUNCTION_TYPE_RUNTIME:
      return call_runtime_function(ctx, entry->data_function.runtime_node, arguments,
                                   arguments_length);
  }
}

static eval_result_internal_t eval_expr(eval_context_t *ctx, ast_node_t *node)
{
  switch (node->type)
  {
    case AST_NODE_PRIMITIVE_EXPR:
      return eval_primitive_expr(ctx, node);
      break;
    case AST_NODE_UNARY_EXPR:
      return eval_unary_expr(ctx, node);
      break;
    case AST_NODE_BINARY_EXPR:
      return eval_binary_expr(ctx, node);
      break;
    case AST_NODE_FUNCTION_CALL_EXPR:
      return eval_function_call_expr(ctx, node);
      break;
    default:
      UNREACHABLE;
  }
}

static eval_result_internal_t eval_primitive_expr(eval_context_t *ctx, ast_node_t *node)
{
  switch (node->data_primitive_expr.token->type)
  {
    case TOKEN_IDENTIFIER:
    {
      char *identifier = node->data_primitive_expr.token->identifier;

      symbol_table_entry_t *entry;
      eval_result_internal_t lookup_result =
        eval_context_lookup_variable(ctx, identifier, &entry);
      if (lookup_result.type != EVAL_SUCCESS)
        return lookup_result;

      return result_success_expr(entry->data_variable.value);
    }
    case TOKEN_CONSTANT:
      return result_success_expr(value_int(node->data_primitive_expr.token->constant));
    default:
      UNREACHABLE;
  }
}

static eval_result_internal_t eval_unary_expr(eval_context_t *ctx, ast_node_t *node)
{
  eval_result_internal_t operand_result = eval_expr(ctx, node->data_unary_expr.operand);
  if (operand_result.type != EVAL_SUCCESS)
    return operand_result;

  switch (node->data_unary_expr.operator)
  {
    case UNARY_OPERATOR_LOGICAL_NEGATION:
      return result_success_expr(value_int(!operand_result.value.value_int));
      break;
    default:
      UNREACHABLE;
  }
}

static eval_result_internal_t eval_binary_expr(eval_context_t *ctx, ast_node_t *node)
{
  binary_operator_t operator = node->data_binary_expr.operator;

  ast_node_t *lhs_node = node->data_binary_expr.left_operand,
             *rhs_node = node->data_binary_expr.right_operand;

  // TODO: Refactor assignment expression to a separate AST node type
  if (operator == BINARY_OPERATOR_ASSIGNMENT)
  {
    char *identifier = lhs_node->data_primitive_expr.token->identifier;
    symbol_table_entry_t *entry;
    eval_result_internal_t lookup_result = eval_context_lookup_variable(ctx, identifier, &entry);
    if (lookup_result.type != EVAL_SUCCESS)
      return lookup_result;

    eval_result_internal_t rhs_result = eval_expr(ctx, rhs_node);
    if (rhs_result.type != EVAL_SUCCESS)
      return rhs_result;

    entry->data_variable.value = rhs_result.value;
    return rhs_result;
  }

  if (operator == BINARY_OPERATOR_LOGICAL_AND)
  {
    eval_result_internal_t lhs_result = eval_expr(ctx, lhs_node);
    if (lhs_result.type != EVAL_SUCCESS)
      return lhs_result;

    if (lhs_result.value.value_int == 0)
      return result_success_expr(value_int(0));
    else
    {
      eval_result_internal_t rhs_result = eval_expr(ctx, rhs_node);
      if (rhs_result.type != EVAL_SUCCESS)
        return rhs_result;

      return result_success_expr(value_int(rhs_result.value.value_int != 0));
    }
  }
  else if (operator == BINARY_OPERATOR_LOGICAL_OR)
  {
    eval_result_internal_t lhs_result = eval_expr(ctx, lhs_node);
    if (lhs_result.type != EVAL_SUCCESS)
      return lhs_result;

    if (lhs_result.value.value_int != 0)
      return result_success_expr(value_int(1));
    else
    {
      eval_result_internal_t rhs_result = eval_expr(ctx, rhs_node);
      if (rhs_result.type != EVAL_SUCCESS)
        return rhs_result;

      return result_success_expr(value_int(rhs_result.value.value_int != 0));
    }
  }
  else
  {
    int lhs_value, rhs_value, result_value_int;

    eval_result_internal_t lhs_result = eval_expr(ctx, lhs_node);
    if (lhs_result.type != EVAL_SUCCESS)
      return lhs_result;

    eval_result_internal_t rhs_result = eval_expr(ctx, rhs_node);
    if (rhs_result.type != EVAL_SUCCESS)
      return rhs_result;

    lhs_value = lhs_result.value.value_int;
    rhs_value = rhs_result.value.value_int;

    switch (operator)
    {
      case BINARY_OPERATOR_ADDITION:
        result_value_int = lhs_value + rhs_value;
        break;
      case BINARY_OPERATOR_SUBTRACTION:
        result_value_int = lhs_value - rhs_value;
        break;
      case BINARY_OPERATOR_MULTIPLICATION:
        result_value_int = lhs_value * rhs_value;
        break;
      case BINARY_OPERATOR_DIVISION:
        if (rhs_value == 0)
          return RESULT_RUNTIME_ERROR("cannot perform division by zero");
        result_value_int = lhs_value / rhs_value;
        break;
      case BINARY_OPERATOR_MODULO:
        if (rhs_value == 0)
          return RESULT_RUNTIME_ERROR("cannot perform modulo by zero");
        result_value_int = lhs_value % rhs_value;
        break;
      case BINARY_OPERATOR_LESS_THAN:
        result_value_int = lhs_value < rhs_value;
        break;
      case BINARY_OPERATOR_GREATER_THAN:
        result_value_int = lhs_value > rhs_value;
        break;
      case BINARY_OPERATOR_LESS_THAN_EQUAL_TO:
        result_value_int = lhs_value <= rhs_value;
        break;
      case BINARY_OPERATOR_GREATER_THAN_EQUAL_TO:
        result_value_int = lhs_value >= rhs_value;
        break;
      case BINARY_OPERATOR_EQUALS:
        result_value_int = lhs_value == rhs_value;
        break;
      case BINARY_OPERATOR_NOT_EQUALS:
        result_value_int = lhs_value != rhs_value;
        break;
      default:
        UNREACHABLE;
    }

    return result_success_expr(value_int(result_value_int));
  }
}

static eval_result_internal_t eval_function_call_expr(eval_context_t *ctx, ast_node_t *node)
{
  char *function_identifier = node->data_function_call_expr.token_callee->identifier;

  symbol_table_entry_t *symbol_table_function_entry;
  eval_result_internal_t lookup_result =
    eval_context_lookup_function(ctx, function_identifier, &symbol_table_function_entry);
  if (lookup_result.type != EVAL_SUCCESS)
    return lookup_result;

  VECTOR_T(ast_node_t*) *argument_nodes = node->data_function_call_expr.arguments;
  eval_value_t *argument_values = NULL;
  if (argument_nodes->length > 0)
  {
    argument_values = malloc(sizeof(eval_value_t) * argument_nodes->length);
    if (argument_values == NULL)
      return RESULT_OOM;
  }

  for (int i = 0; i < argument_nodes->length; i++)
  {
    eval_result_internal_t argument_result =
      eval_expr(ctx, VECTOR_ARR(argument_nodes, ast_node_t*)[i]);
    if (argument_result.type != EVAL_SUCCESS)
    {
      free(argument_values);
      return argument_result;
    }

    argument_values[i] = argument_result.value;
  }

  eval_result_internal_t call_result =
    call_function(ctx, symbol_table_function_entry, argument_values, argument_nodes->length);
  free(argument_values);

  return call_result;
}

static eval_result_internal_t eval_stmt_list(eval_context_t *ctx, ast_node_t *node)
{
  ast_node_t **stmts_arr = VECTOR_ARR(node->data_stmt_list.stmts, ast_node_t*);

  if (!eval_context_enter_new_env(ctx, ctx->current_env))
    return RESULT_OOM;

  for (int i = 0; i < node->data_stmt_list.stmts->length; i++)
  {
    eval_result_internal_t stmt_result;

    switch (stmts_arr[i]->type)
    {
      case AST_NODE_EXPR_STMT:
        stmt_result = eval_expr_stmt(ctx, stmts_arr[i]);
        break;
      case AST_NODE_IF_STMT:
        stmt_result = eval_if_stmt(ctx, stmts_arr[i]);
        break;
      case AST_NODE_WHILE_STMT:
        stmt_result = eval_while_stmt(ctx, stmts_arr[i]);
        break;
      case AST_NODE_RETURN_STMT:
        stmt_result = eval_return_stmt(ctx, stmts_arr[i]);
        break;
      case AST_NODE_VARIABLE_DECL:
        stmt_result = eval_variable_decl(ctx, stmts_arr[i]);
        if (stmt_result.type != EVAL_SUCCESS)
        {
          eval_context_exit_current_env(ctx);
          return stmt_result;
        }

        stmt_result = RESULT_SUCCESS_STMT_NO_RETURN;
        break;
      default:
        UNREACHABLE;
    }

    if (stmt_result.type != EVAL_SUCCESS || stmt_result.returned)
    {
      eval_context_exit_current_env(ctx);
      return stmt_result;
    }
  }

  eval_context_exit_current_env(ctx);
  return RESULT_SUCCESS_STMT_NO_RETURN;
}

static eval_result_internal_t eval_expr_stmt(eval_context_t *ctx, ast_node_t *node)
{
  eval_result_internal_t expr_result = eval_expr(ctx, node->data_expr_stmt.expr);
  if (expr_result.type != EVAL_SUCCESS)
    return expr_result;

  return RESULT_SUCCESS_STMT_NO_RETURN;
}

static eval_result_internal_t eval_if_stmt(eval_context_t *ctx, ast_node_t *node)
{
  eval_result_internal_t cond_result = eval_expr(ctx, node->data_if_stmt.cond_expr);
  if (cond_result.type != EVAL_SUCCESS)
    return cond_result;

  if (cond_result.value.value_int != 0)
    return eval_stmt_list(ctx, node->data_if_stmt.stmt_list);

  return RESULT_SUCCESS_STMT_NO_RETURN;
}

static eval_result_internal_t eval_while_stmt(eval_context_t *ctx, ast_node_t *node)
{
  eval_result_internal_t cond_result = eval_expr(ctx, node->data_while_stmt.cond_expr);
  if (cond_result.type != EVAL_SUCCESS)
    return cond_result;

  while (cond_result.value.value_int != 0)
  {
    eval_result_internal_t body_result = eval_stmt_list(ctx, node->data_while_stmt.stmt_list);
    if (body_result.type != EVAL_SUCCESS || body_result.returned)
      return body_result;

    cond_result = eval_expr(ctx, node->data_while_stmt.cond_expr);
    if (cond_result.type != EVAL_SUCCESS)
      return cond_result;
  }

  return RESULT_SUCCESS_STMT_NO_RETURN;
}

static eval_result_internal_t eval_return_stmt(eval_context_t *ctx, ast_node_t *node)
{
  eval_result_internal_t returned = eval_expr(ctx, node->data_return_stmt.expr);
  if (returned.type != EVAL_SUCCESS)
    return returned;

  return (eval_result_internal_t) {
    .type = EVAL_SUCCESS,
    .returned = 1,
    .value = returned.value,
  };
}

static eval_result_internal_t eval_variable_decl(eval_context_t *ctx, ast_node_t *node)
{
  symbol_table_entry_t *symbol_table_entry = malloc(sizeof(symbol_table_entry_t));
  if (symbol_table_entry == NULL)
    return RESULT_OOM;

  symbol_table_entry->symbol_identifier = node->data_variable_decl.token_identifier->identifier;
  symbol_table_entry->symbol_type = SYMBOL_TYPE_VARIABLE;

  if (node->data_variable_decl.initializer == NULL)
  {
    symbol_table_entry->data_variable.value = value_int(0);
  }
  else
  {
    eval_result_internal_t initializer_result =
      eval_expr(ctx, node->data_variable_decl.initializer);

    if (initializer_result.type != EVAL_SUCCESS)
    {
      symbol_table_entry_free(symbol_table_entry);
      return initializer_result;
    }

    symbol_table_entry->data_variable.value = initializer_result.value;
  }

  eval_result_internal_t add_result =
    eval_env_add_symbol_table_entry(ctx->current_env, symbol_table_entry);
  if (add_result.type != EVAL_SUCCESS)
  {
    symbol_table_entry_free(symbol_table_entry);
    return add_result;
  }

  return RESULT_SUCCESS;
}

static eval_result_internal_t eval_function_decl(eval_context_t *ctx, ast_node_t *node)
{
  symbol_table_entry_t *symbol_table_entry = malloc(sizeof(symbol_table_entry_t));
  if (symbol_table_entry == NULL)
    return RESULT_OOM;

  symbol_table_entry->symbol_identifier = node->data_function_decl.token_identifier->identifier;
  symbol_table_entry->symbol_type = SYMBOL_TYPE_FUNCTION;
  symbol_table_entry->data_function.type = FUNCTION_TYPE_RUNTIME;
  symbol_table_entry->data_function.runtime_node = node;

  eval_result_internal_t add_result =
    eval_env_add_symbol_table_entry(ctx->current_env, symbol_table_entry);
  if (add_result.type != EVAL_SUCCESS)
  {
    symbol_table_entry_free(symbol_table_entry);
    return add_result;
  }

  return RESULT_SUCCESS;
}

// The output contract is the same as that of expression evaluators.
static eval_result_internal_t eval_program(eval_context_t *ctx, ast_node_t *node)
{
  ast_node_t *node_main_function_decl = NULL;
  for (int i = 0; i < node->data_program.decls->length; i++)
  {
    eval_result_internal_t decl_result;
    ast_node_t *node_top_level_decl = VECTOR_ARR(node->data_program.decls, ast_node_t*)[i];

    switch (node_top_level_decl->type)
    {
      case AST_NODE_VARIABLE_DECL:
        decl_result = eval_variable_decl(ctx, node_top_level_decl);
        break;
      case AST_NODE_FUNCTION_DECL:
        if (!strcmp(node_top_level_decl->data_function_decl.token_identifier->identifier, "main"))
          node_main_function_decl = node_top_level_decl;
        decl_result = eval_function_decl(ctx, node_top_level_decl);
        break;
      default:
        UNREACHABLE;
    }

    if (decl_result.type != EVAL_SUCCESS)
      return decl_result;
  }

  if (node_main_function_decl == NULL)
    RESULT_RUNTIME_ERROR("programs should have the main function");

  // TODO: Add some arguments to main()
  return call_runtime_function(ctx, node_main_function_decl, NULL, 0);
}

eval_result_t evaluate(ast_node_t *node)
{
  if (node->type != AST_NODE_PROGRAM)
    exit_with_error("evaluate(): 'node' must be a node with type 'AST_NODE_PROGRAM'");

  eval_context_t *ctx = eval_context_create();
  if (ctx == NULL)
    return (eval_result_t) {
      .type = EVAL_OOM,
    };

  eval_result_internal_t result = eval_program(ctx, node);

  eval_context_free(ctx);

  if (result.type != EVAL_SUCCESS)
  {
    return (eval_result_t) {
      .type = result.type,
      .error_message = result.error_message,
    };
  }

  return (eval_result_t) {
    .type = result.type,
    .value = result.value,
  };
}
