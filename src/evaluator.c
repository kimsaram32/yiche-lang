#include <stdio.h>
#include <string.h>
#include "yiche.h"

/*
 * Types
 */

typedef struct evaluate_env_t
{
  HASH_TABLE_T(symbol_table_entry_t) *symbol_table;
  struct evaluate_env_t *parent, *prev;
}
evaluate_env_t;

typedef struct
{
  hash_table_t *builtin_symbol_table;
  evaluate_env_t *current_env, *global_env;
}
evaluate_context_t;

typedef enum
{
  SYMBOL_TYPE_VARIABLE,
  SYMBOL_TYPE_FUNCTION,
}
symbol_type_t;

typedef struct
{
  evaluate_value_t value;
}
symbol_table_entry_variable_t;

typedef enum
{
  FUNCTION_TYPE_RUNTIME,
  FUNCTION_TYPE_BUILTIN,
}
function_type_t;

// Accesses to 'args + i' with '0 <= i < builtin_params_length' is safe.
typedef evaluate_value_t (*builtin_function_t)(evaluate_context_t *ctx, evaluate_value_t *args);

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
static evaluate_env_t *evaluate_env_create(evaluate_env_t *parent, evaluate_env_t *prev)
{
  evaluate_env_t *env = malloc(sizeof(evaluate_env_t));
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
static void evaluate_env_free(evaluate_env_t *env)
{
  hash_table_free(env->symbol_table);
  free(env);
}

static void evaluate_env_add_symbol_table_entry(evaluate_env_t *env, symbol_table_entry_t *entry)
{
  if (hash_table_search(env->symbol_table, entry->symbol_identifier))
    exit_runtime_error("identifier '%s' already declared in the current scope", entry->symbol_identifier);

  if (!hash_table_insert(env->symbol_table, entry->symbol_identifier, entry))
    exit_out_of_memory();
}

/*
 * Built-in functions
 */

static evaluate_value_t builtin_function_print(evaluate_context_t *ctx,
                                               evaluate_value_t *arguments)
{
  evaluate_value_t n = arguments[0];

  printf("%d", n.value_int);

  return (evaluate_value_t){
    .data_type = DATA_TYPE_INT,
    .value_int = 0,
  };
}

static evaluate_value_t builtin_function_println(evaluate_context_t *ctx,
                                                  evaluate_value_t *arguments)
{
  evaluate_value_t n = arguments[0];

  printf("%d\n", n.value_int);

  return (evaluate_value_t){
    .data_type = DATA_TYPE_INT,
    .value_int = 0,
  };
}

static evaluate_value_t builtin_function_print_char(evaluate_context_t *ctx,
                                                    evaluate_value_t *arguments)
{
  evaluate_value_t n = arguments[0];

  printf("%c", n.value_int);

  return (evaluate_value_t){
    .data_type = DATA_TYPE_INT,
    .value_int = 0,
  };
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
static void builtin_add_symbol_table_entries(hash_table_t *table)
{
  for (int i = 0; i < ARRAY_LENGTH(builtin_symbol_table_entries); i++)
  {
    symbol_table_entry_t *entry = malloc(sizeof(symbol_table_entry_t));
    if (entry == NULL)
      exit_out_of_memory();

    memcpy(entry, builtin_symbol_table_entries + i, sizeof(symbol_table_entry_t));

    if (!hash_table_insert(table, entry->symbol_identifier, entry))
      exit_out_of_memory();
  }
}

/*
 * Evaluation context
 */

static evaluate_context_t *evaluate_context_create(void)
{
  evaluate_context_t *ctx = malloc(sizeof(evaluate_context_t));
  if (ctx == NULL)
    return NULL;

  if ((ctx->builtin_symbol_table = symbol_table_create()) == NULL)
  {
    free(ctx);
    return NULL;
  }

  if ((ctx->global_env = evaluate_env_create(NULL, NULL)) == NULL)
  {
    hash_table_free(ctx->builtin_symbol_table);
    free(ctx);
    return NULL;
  }

  ctx->current_env = ctx->global_env;

  builtin_add_symbol_table_entries(ctx->builtin_symbol_table);

  return ctx;
}

static void evaluate_context_free(evaluate_context_t *ctx)
{
  evaluate_env_t *env = ctx->current_env, *next_env;

  hash_table_free(ctx->builtin_symbol_table);

  while (env != NULL)
  {
    next_env = env->prev;
    evaluate_env_free(env);
    env = next_env;
  }

  free(ctx);
}

static void evaluate_context_enter_new_env(evaluate_context_t *ctx, evaluate_env_t *parent)
{
  evaluate_env_t *new_env = evaluate_env_create(parent, ctx->current_env);
  if (new_env == NULL)
    exit_out_of_memory();

  ctx->current_env = new_env;
}

static void evaluate_context_exit_current_env(evaluate_context_t *ctx)
{
  if (ctx->current_env == ctx->global_env)
    exit_with_error("evaluate_context_exit_current_env(): cannot exit the global environment");

  evaluate_env_t *prev_env = ctx->current_env->prev;
  evaluate_env_free(ctx->current_env);
  ctx->current_env = prev_env;
}

static symbol_table_entry_t *evaluate_context_lookup_symbol_table_entry(evaluate_context_t *ctx,
                                                                        char *symbol_identifier)
{
  evaluate_env_t *env = ctx->current_env;
  while (env != NULL)
  {
    symbol_table_entry_t *entry = hash_table_search(env->symbol_table, symbol_identifier);
    if (entry != NULL)
      return entry;
    env = env->parent;
  }
  return hash_table_search(ctx->builtin_symbol_table, symbol_identifier);
}

static symbol_table_entry_t *evaluate_context_lookup_variable(evaluate_context_t *ctx,
                                                              char *symbol_identifier)
{
  symbol_table_entry_t *entry = evaluate_context_lookup_symbol_table_entry(ctx, symbol_identifier);

  if (entry == NULL)
    exit_runtime_error("undeclared identifier '%s'", symbol_identifier);
  else if (entry->symbol_type != SYMBOL_TYPE_VARIABLE)
    exit_runtime_error("identifier '%s' is not a variable", symbol_identifier);

  return entry;
}

static symbol_table_entry_t *evaluate_context_lookup_function(evaluate_context_t *ctx,
                                                              char *symbol_identifier)
{
  symbol_table_entry_t *entry = evaluate_context_lookup_symbol_table_entry(ctx, symbol_identifier);

  if (entry == NULL)
    exit_runtime_error("undeclared identifier '%s'", symbol_identifier);
  else if (entry->symbol_type != SYMBOL_TYPE_FUNCTION)
    exit_runtime_error("identifier '%s' is not a function", symbol_identifier);

  return entry;
}

/*
 * Evaluation
 */

typedef struct
{
  int returned;
  evaluate_value_t returned_value;
}
evaluate_stmt_result_t;

static evaluate_stmt_result_t EVALUATE_STMT_RESULT_NO_RETURN = {
  .returned = 0,
};

static evaluate_value_t evaluate_primitive_expr(evaluate_context_t *ctx, ast_node_t *node);
static evaluate_value_t evaluate_unary_expr(evaluate_context_t *ctx, ast_node_t *node);
static evaluate_value_t evaluate_binary_expr(evaluate_context_t *ctx, ast_node_t *node);
static evaluate_value_t evaluate_function_call_expr(evaluate_context_t *ctx, ast_node_t *node);

static evaluate_stmt_result_t evaluate_expr_stmt(evaluate_context_t *ctx, ast_node_t *node);
static evaluate_stmt_result_t evaluate_if_stmt(evaluate_context_t *ctx, ast_node_t *node);
static evaluate_stmt_result_t evaluate_while_stmt(evaluate_context_t *ctx, ast_node_t *node);
static evaluate_stmt_result_t evaluate_return_stmt(evaluate_context_t *ctx, ast_node_t *node);
static evaluate_stmt_result_t evaluate_stmt_list(evaluate_context_t *ctx, ast_node_t *node);

static void evaluate_variable_decl(evaluate_context_t *ctx, ast_node_t *node);
static void evaluate_function_decl(evaluate_context_t *ctx, ast_node_t *node);

static void assert_argument_count_match(int expected, int passed, char *identifier)
{
  if (expected != passed)
    exit_runtime_error("function call argument mismatch: '%s' expects %d argument(s),"
                       " but %d got passed", identifier, expected, passed);
}

static evaluate_value_t call_builtin_function(evaluate_context_t *ctx, symbol_table_entry_t *entry,
                                              VECTOR_T(evaluate_value_t) *arguments)
{
  evaluate_value_t *arguments_arr = VECTOR_ARR(arguments, evaluate_value_t);

  assert_argument_count_match(entry->data_function.builtin_parameters_length,
                              arguments->length,
                              entry->symbol_identifier);

  return entry->data_function.builtin_function(ctx, arguments_arr);
}

static evaluate_value_t call_runtime_function(evaluate_context_t *ctx, ast_node_t *runtime_node,
                                              VECTOR_T(evaluate_value_t) *arguments)
{
  ast_node_function_decl_t function_data = runtime_node->data_function_decl;
  ast_node_t **parameters_arr = VECTOR_ARR(function_data.parameters, ast_node_t*);
  evaluate_value_t *arguments_arr = VECTOR_ARR(arguments, evaluate_value_t);

  assert_argument_count_match(function_data.parameters->length,
                              arguments->length,
                              function_data.token_identifier->identifier);

  evaluate_context_enter_new_env(ctx, ctx->global_env);

  for (int i = 0; i < arguments->length; i++)
  {
    char *param_identifier = parameters_arr[i]->data_variable_decl.token_identifier->identifier;
    // TODO: check data types when proper support is added

    symbol_table_entry_t *symbol_table_entry = malloc(sizeof(symbol_table_entry_t));
    if (symbol_table_entry == NULL)
      exit_out_of_memory();

    symbol_table_entry->symbol_identifier = param_identifier;
    symbol_table_entry->symbol_type = SYMBOL_TYPE_VARIABLE;
    symbol_table_entry->data_variable.value = arguments_arr[i];

    evaluate_env_add_symbol_table_entry(ctx->current_env, symbol_table_entry);
  }

  evaluate_stmt_result_t result = evaluate_stmt_list(ctx, runtime_node->data_function_decl.body);

  evaluate_context_exit_current_env(ctx);

  if (!result.returned)
    exit_runtime_error("function '%s' does not return a value",
                       runtime_node->data_function_decl.token_identifier->identifier);

  return result.returned_value;
}

static evaluate_value_t call_function(evaluate_context_t *ctx, symbol_table_entry_t *entry,
                                      VECTOR_T(evaluate_value_t) *arguments)
{
  switch (entry->data_function.type)
  {
    case FUNCTION_TYPE_BUILTIN:
      return call_builtin_function(ctx, entry, arguments);
    case FUNCTION_TYPE_RUNTIME:
      return call_runtime_function(ctx, entry->data_function.runtime_node, arguments);
  }
}

static evaluate_value_t evaluate_expr(evaluate_context_t *ctx, ast_node_t *node)
{
  switch (node->type)
  {
    case AST_NODE_PRIMITIVE_EXPR:
      return evaluate_primitive_expr(ctx, node);
      break;
    case AST_NODE_UNARY_EXPR:
      return evaluate_unary_expr(ctx, node);
      break;
    case AST_NODE_BINARY_EXPR:
      return evaluate_binary_expr(ctx, node);
      break;
    case AST_NODE_FUNCTION_CALL_EXPR:
      return evaluate_function_call_expr(ctx, node);
      break;
    default:
      UNREACHABLE;
  }
}

static evaluate_value_t evaluate_primitive_expr(evaluate_context_t *ctx, ast_node_t *node)
{
  evaluate_value_t result;

  result.data_type = DATA_TYPE_INT;

  switch (node->data_primitive_expr.token->type)
  {
    case TOKEN_IDENTIFIER:
    {
      char *identifier = node->data_primitive_expr.token->identifier;
      symbol_table_entry_t *entry = evaluate_context_lookup_variable(ctx, identifier);

      result.value_int = entry->data_variable.value.value_int;
      break;
    }
    case TOKEN_CONSTANT:
    {
      result.value_int = node->data_primitive_expr.token->constant;
      break;
    }
    default:
      UNREACHABLE;
  }

  return result;
}

static evaluate_value_t evaluate_unary_expr(evaluate_context_t *ctx, ast_node_t *node)
{
  evaluate_value_t result = evaluate_expr(ctx, node->data_unary_expr.operand);

  switch (node->data_unary_expr.operator)
  {
    case UNARY_OPERATOR_LOGICAL_NEGATION:
      result.value_int = !result.value_int;
      break;
    default:
      UNREACHABLE;
  }

  return result;
}

static evaluate_value_t evaluate_binary_expr(evaluate_context_t *ctx, ast_node_t *node)
{
  binary_operator_t operator = node->data_binary_expr.operator;

  ast_node_t *lhs_node = node->data_binary_expr.left_operand,
             *rhs_node = node->data_binary_expr.right_operand;

  // TODO: Refactor assignment expression to a separate AST node type
  if (operator == BINARY_OPERATOR_ASSIGNMENT)
  {
    char *identifier = lhs_node->data_primitive_expr.token->identifier;
    symbol_table_entry_t *entry = evaluate_context_lookup_variable(ctx, identifier);

    entry->data_variable.value = evaluate_expr(ctx, rhs_node);
    return entry->data_variable.value;
  }

  evaluate_value_t result = {
    .data_type = DATA_TYPE_INT,
  };

  if (operator == BINARY_OPERATOR_LOGICAL_AND)
  {
    result.value_int = evaluate_expr(ctx, lhs_node).value_int
                       && evaluate_expr(ctx, rhs_node).value_int;
  }
  else if (operator == BINARY_OPERATOR_LOGICAL_OR)
  {
    result.value_int = evaluate_expr(ctx, lhs_node).value_int
                       || evaluate_expr(ctx, rhs_node).value_int;
  }
  else
  {
    int lhs_value = evaluate_expr(ctx, lhs_node).value_int,
        rhs_value = evaluate_expr(ctx, rhs_node).value_int;

    switch (operator)
    {
      case BINARY_OPERATOR_ADDITION:
        result.value_int = lhs_value + rhs_value;
        break;
      case BINARY_OPERATOR_SUBTRACTION:
        result.value_int = lhs_value - rhs_value;
        break;
      case BINARY_OPERATOR_MULTIPLICATION:
        result.value_int = lhs_value * rhs_value;
        break;
      case BINARY_OPERATOR_DIVISION:
        if (rhs_value == 0)
          exit_runtime_error("cannot perform division by zero");
        result.value_int = lhs_value / rhs_value;
        break;
      case BINARY_OPERATOR_MODULO:
        if (rhs_value == 0)
          exit_runtime_error("cannot perform modulo by zero");
        result.value_int = lhs_value % rhs_value;
        break;
      case BINARY_OPERATOR_LESS_THAN:
        result.value_int = lhs_value < rhs_value;
        break;
      case BINARY_OPERATOR_GREATER_THAN:
        result.value_int = lhs_value > rhs_value;
        break;
      case BINARY_OPERATOR_LESS_THAN_EQUAL_TO:
        result.value_int = lhs_value <= rhs_value;
        break;
      case BINARY_OPERATOR_GREATER_THAN_EQUAL_TO:
        result.value_int = lhs_value >= rhs_value;
        break;
      case BINARY_OPERATOR_EQUALS:
        result.value_int = lhs_value == rhs_value;
        break;
      case BINARY_OPERATOR_NOT_EQUALS:
        result.value_int = lhs_value != rhs_value;
        break;
      default:
        UNREACHABLE;
    }
  }

  return result;
}

static evaluate_value_t evaluate_function_call_expr(evaluate_context_t *ctx, ast_node_t *node)
{
  char *function_identifier = node->data_function_call_expr.token_callee->identifier;
  symbol_table_entry_t *symbol_table_function_entry =
    evaluate_context_lookup_function(ctx, function_identifier);

  VECTOR_T(evaluate_value_t) *argument_values = vector_create(sizeof(evaluate_value_t), 8);
  VECTOR_T(ast_node_t*) *argument_nodes = node->data_function_call_expr.arguments;
  for (int i = 0; i < argument_nodes->length; i++)
  {
    evaluate_value_t argument_value = evaluate_expr(ctx, VECTOR_ARR(argument_nodes, ast_node_t*)[i]);
    evaluate_value_t *argument_value_elt = vector_next_element(argument_values);
    memcpy(argument_value_elt, &argument_value, sizeof(evaluate_value_t));
  }

  evaluate_value_t returned_value =
    call_function(ctx, symbol_table_function_entry, argument_values);

  vector_free(argument_values);
  return returned_value;
}

static evaluate_stmt_result_t evaluate_stmt_list(evaluate_context_t *ctx, ast_node_t *node)
{
  ast_node_t **stmts_arr = VECTOR_ARR(node->data_stmt_list.stmts, ast_node_t*);

  evaluate_context_enter_new_env(ctx, ctx->current_env);
  for (int i = 0; i < node->data_stmt_list.stmts->length; i++)
  {
    evaluate_stmt_result_t result;

    switch (stmts_arr[i]->type)
    {
      case AST_NODE_EXPR_STMT:
        result = evaluate_expr_stmt(ctx, stmts_arr[i]);
        break;
      case AST_NODE_IF_STMT:
        result = evaluate_if_stmt(ctx, stmts_arr[i]);
        break;
      case AST_NODE_WHILE_STMT:
        result = evaluate_while_stmt(ctx, stmts_arr[i]);
        break;
      case AST_NODE_RETURN_STMT:
        result = evaluate_return_stmt(ctx, stmts_arr[i]);
        break;
      case AST_NODE_VARIABLE_DECL:
        evaluate_variable_decl(ctx, stmts_arr[i]);
        result = EVALUATE_STMT_RESULT_NO_RETURN;
        break;
      default:
        UNREACHABLE;
    }

    if (result.returned)
    {
      evaluate_context_exit_current_env(ctx);
      return result;
    }
  }

  evaluate_context_exit_current_env(ctx);
  return EVALUATE_STMT_RESULT_NO_RETURN;
}

static evaluate_stmt_result_t evaluate_expr_stmt(evaluate_context_t *ctx, ast_node_t *node)
{
  evaluate_expr(ctx, node->data_expr_stmt.expr);
  return EVALUATE_STMT_RESULT_NO_RETURN;
}

static evaluate_stmt_result_t evaluate_if_stmt(evaluate_context_t *ctx, ast_node_t *node)
{
  evaluate_value_t cond = evaluate_expr(ctx, node->data_if_stmt.cond_expr);

  if (cond.value_int != 0)
     return evaluate_stmt_list(ctx, node->data_if_stmt.stmt_list);

  return EVALUATE_STMT_RESULT_NO_RETURN;
}

static evaluate_stmt_result_t evaluate_while_stmt(evaluate_context_t *ctx, ast_node_t *node)
{
  evaluate_value_t cond = evaluate_expr(ctx, node->data_while_stmt.cond_expr);

  while (cond.value_int != 0)
  {
    evaluate_stmt_result_t result = evaluate_stmt_list(ctx, node->data_while_stmt.stmt_list);
    if (result.returned)
      return result;
    cond = evaluate_expr(ctx, node->data_while_stmt.cond_expr);
  }

  return EVALUATE_STMT_RESULT_NO_RETURN;
}

static evaluate_stmt_result_t evaluate_return_stmt(evaluate_context_t *ctx, ast_node_t *node)
{
  evaluate_stmt_result_t result = {
    .returned = 1,
    .returned_value = evaluate_expr(ctx, node->data_return_stmt.expr),
  };
  return result;
}

static void evaluate_variable_decl(evaluate_context_t *ctx, ast_node_t *node)
{
  symbol_table_entry_t *symbol_table_entry = malloc(sizeof(symbol_table_entry_t));
  if (symbol_table_entry == NULL)
    exit_out_of_memory();

  symbol_table_entry->symbol_identifier = node->data_variable_decl.token_identifier->identifier;
  symbol_table_entry->symbol_type = SYMBOL_TYPE_VARIABLE;

  if (node->data_variable_decl.initializer == NULL)
  {
    symbol_table_entry->data_variable.value = (evaluate_value_t){
      .data_type = DATA_TYPE_INT,
      .value_int = 0,
    };
  }
  else
    symbol_table_entry->data_variable.value = evaluate_expr(ctx, node->data_variable_decl.initializer);

  evaluate_env_add_symbol_table_entry(ctx->current_env, symbol_table_entry);
}

static void evaluate_function_decl(evaluate_context_t *ctx, ast_node_t *node)
{
  symbol_table_entry_t *symbol_table_entry = malloc(sizeof(symbol_table_entry_t));
  if (symbol_table_entry == NULL)
    exit_out_of_memory();

  symbol_table_entry->symbol_identifier = node->data_function_decl.token_identifier->identifier;
  symbol_table_entry->symbol_type = SYMBOL_TYPE_FUNCTION;
  symbol_table_entry->data_function.type = FUNCTION_TYPE_RUNTIME;
  symbol_table_entry->data_function.runtime_node = node;

  evaluate_env_add_symbol_table_entry(ctx->current_env, symbol_table_entry);
}

static evaluate_value_t evaluate_program(ast_node_t *node)
{
  evaluate_context_t *ctx = evaluate_context_create();
  if (ctx == NULL)
    exit_out_of_memory();

  ast_node_t *node_main_function_decl = NULL;

  for (int i = 0; i < node->data_program.decls->length; i++)
  {
    ast_node_t *node_top_level_decl = VECTOR_ARR(node->data_program.decls, ast_node_t*)[i];
    switch (node_top_level_decl->type)
    {
      case AST_NODE_VARIABLE_DECL:
        evaluate_variable_decl(ctx, node_top_level_decl);
        break;
      case AST_NODE_FUNCTION_DECL:
        if (!strcmp(node_top_level_decl->data_function_decl.token_identifier->identifier, "main"))
          node_main_function_decl = node_top_level_decl;
        evaluate_function_decl(ctx, node_top_level_decl);
        break;
      default:
        UNREACHABLE;
    }
  }

  if (node_main_function_decl == NULL)
    exit_runtime_error("programs should have the main function");

  VECTOR_T(evaluate_value_t) *arguments = vector_create(sizeof(evaluate_value_t), 8);
  // TODO: Add some arguments to main()
  evaluate_value_t result = call_runtime_function(ctx, node_main_function_decl, arguments);
  vector_free(arguments);

  evaluate_context_free(ctx);
  return result;
}

evaluate_value_t evaluate(ast_node_t *node)
{
  return evaluate_program(node);
}
