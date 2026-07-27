#include <stdio.h>
#include <string.h>
#include "yiche.h"

/*
 * Symbol tables and execution environments
 */

typedef enum
{
  SYMBOL_TYPE_VARIABLE,
  SYMBOL_TYPE_FUNCTION,
}
symbol_type_t;

typedef struct
{
  char *symbol_identifier;
  symbol_type_t symbol_type;
  evaluate_value_t variable_value;
  ast_node_t *function_node;
}
symbol_table_entry_t;

static void symbol_table_entry_free(symbol_table_entry_t *entry)
{
  free(entry);
}

typedef struct evaluate_env_t
{
  HASH_TABLE_T(symbol_table_entry_t) *symbol_table;
  struct evaluate_env_t *parent;
}
evaluate_env_t;

#define EVALUATE_ENV_SYMBOL_TABLE_INITIAL_CAPACITY 107

static evaluate_env_t *evaluate_env_create(evaluate_env_t *parent)
{
  evaluate_env_t *env = malloc(sizeof(evaluate_env_t));
  if (env == NULL)
    return NULL;

  env->symbol_table = hash_table_create(EVALUATE_ENV_SYMBOL_TABLE_INITIAL_CAPACITY,
                                        (payload_destructor_t)symbol_table_entry_free);
  if (env->symbol_table == NULL)
  {
    free(env);
    return NULL;
  }

  env->parent = parent;

  return env;
}

// The parent environment ('env->parent') is not released.
static void evaluate_env_free(evaluate_env_t *env)
{
  hash_table_free(env->symbol_table);
  free(env);
}

static symbol_table_entry_t *evaluate_env_lookup_symbol_table_entry(evaluate_env_t *env, char *symbol_identifier)
{
  while (env != NULL)
  {
    symbol_table_entry_t *entry = hash_table_search(env->symbol_table, symbol_identifier);
    if (entry != NULL)
      return entry;
    env = env->parent;
  }
  return NULL;
}

static symbol_table_entry_t *evaluate_env_lookup_variable(evaluate_env_t *env, char *symbol_identifier)
{
  symbol_table_entry_t *entry = evaluate_env_lookup_symbol_table_entry(env, symbol_identifier);

  if (entry == NULL)
    exit_runtime_error("undeclared identifier '%s'", symbol_identifier);
  else if (entry->symbol_type != SYMBOL_TYPE_VARIABLE)
    exit_runtime_error("identifier '%s' is not a variable", symbol_identifier);

  return entry;
}

static symbol_table_entry_t *evaluate_env_lookup_function(evaluate_env_t *env, char *symbol_identifier)
{
  symbol_table_entry_t *entry = evaluate_env_lookup_symbol_table_entry(env, symbol_identifier);

  if (entry == NULL)
    exit_runtime_error("undeclared identifier '%s'", symbol_identifier);
  else if (entry->symbol_type != SYMBOL_TYPE_FUNCTION)
    exit_runtime_error("identifier '%s' is not a function", symbol_identifier);

  return entry;
}

static void evaluate_env_add_symbol_table_entry(evaluate_env_t *env, symbol_table_entry_t *entry)
{
  if (hash_table_search(env->symbol_table, entry->symbol_identifier))
    exit_runtime_error("identifier '%s' already declared in the current scope", entry->symbol_identifier);

  hash_table_insert(env->symbol_table, entry->symbol_identifier, entry);
}

/*
 * Evaluation context
 */

typedef struct
{
  evaluate_env_t *current_env;
}
evaluate_context_t;

static evaluate_context_t *evaluate_context_create(void)
{
  evaluate_context_t *ctx = malloc(sizeof(evaluate_context_t));
  if (ctx == NULL)
    return NULL;

  if ((ctx->current_env = evaluate_env_create(NULL)) == NULL)
  {
    free(ctx);
    return NULL;
  }

  return ctx;
}

static void evaluate_context_free(evaluate_context_t *ctx)
{
  evaluate_env_t *env = ctx->current_env, *next_env;

  while (env != NULL)
  {
    next_env = env->parent;
    evaluate_env_free(env);
    env = next_env;
  }

  free(ctx);
}

static void evaluate_context_enter_new_env(evaluate_context_t *ctx)
{
  evaluate_env_t *new_env = evaluate_env_create(ctx->current_env);
  if (new_env == NULL)
    exit_out_of_memory();

  ctx->current_env = new_env;
}

static void evaluate_context_exit_current_env(evaluate_context_t *ctx)
{
  if (ctx->current_env->parent == NULL)
    exit_with_error("evaluate_context_exit_current_env(): cannot exit the global environment");

  evaluate_env_t *parent_env = ctx->current_env->parent;
  evaluate_env_free(ctx->current_env);
  ctx->current_env = parent_env;
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

static evaluate_value_t call_function(evaluate_context_t *ctx, ast_node_t *function_node,
                                      VECTOR_T(evaluate_value_t) *arguments)
{
  ast_node_function_decl_t function_data = function_node->data_function_decl;
  ast_node_t **parameters_arr = VECTOR_ARR(function_data.parameters, ast_node_t*);
  evaluate_value_t *arguments_arr = VECTOR_ARR(arguments, evaluate_value_t);

  if (function_data.parameters->length != arguments->length)
    exit_runtime_error("function call argument mismatch: '%s' expects %d argument(s),"
                       " but %d got passed",
                       function_data.token_identifier->identifier,
                       function_data.parameters->length,
                       arguments->length);

  evaluate_context_enter_new_env(ctx);

  for (int i = 0; i < arguments->length; i++)
  {
    char *param_identifier = parameters_arr[i]->data_variable_decl.token_identifier->identifier;
    // TODO: check data types when proper support is added

    symbol_table_entry_t *symbol_table_entry = malloc(sizeof(symbol_table_entry_t));
    if (symbol_table_entry == NULL)
      exit_out_of_memory();

    symbol_table_entry->symbol_identifier = param_identifier;
    symbol_table_entry->symbol_type = SYMBOL_TYPE_VARIABLE;
    symbol_table_entry->variable_value = arguments_arr[i];

    evaluate_env_add_symbol_table_entry(ctx->current_env, symbol_table_entry);
  }

  evaluate_stmt_result_t result = evaluate_stmt_list(ctx, function_node->data_function_decl.body);

  evaluate_context_exit_current_env(ctx);

  if (!result.returned)
    exit_runtime_error("function '%s' does not return a value",
                       function_node->data_function_decl.token_identifier->identifier);

  return result.returned_value;
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
      symbol_table_entry_t *entry = evaluate_env_lookup_variable(ctx->current_env, identifier);

      result.value_int = entry->variable_value.value_int;
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
  evaluate_value_t lhs = evaluate_expr(ctx, node->data_binary_expr.left_operand),
                   rhs = evaluate_expr(ctx, node->data_binary_expr.right_operand);

  // TODO: Refactor assignment expression to a separate AST node type
  if (node->data_binary_expr.operator == BINARY_OPERATOR_ASSIGNMENT)
  {
    ast_node_t *left_operand = node->data_binary_expr.left_operand;
    char *identifier = left_operand->data_primitive_expr.token->identifier;
    symbol_table_entry_t *entry = evaluate_env_lookup_variable(ctx->current_env, identifier);
    entry->variable_value = rhs;
    return rhs;
  }

  evaluate_value_t result;
  result.data_type = DATA_TYPE_INT;

  switch (node->data_binary_expr.operator)
  {
    case BINARY_OPERATOR_ADDITION:
      result.value_int = lhs.value_int + rhs.value_int;
      break;
    case BINARY_OPERATOR_SUBTRACTION:
      result.value_int = lhs.value_int - rhs.value_int;
      break;
    case BINARY_OPERATOR_MULTIPLICATION:
      result.value_int = lhs.value_int * rhs.value_int;
      break;
    case BINARY_OPERATOR_DIVISION:
      result.value_int = lhs.value_int / rhs.value_int;
      break;
    case BINARY_OPERATOR_MODULO:
      result.value_int = lhs.value_int % rhs.value_int;
      break;
    case BINARY_OPERATOR_LESS_THAN:
      result.value_int = lhs.value_int < rhs.value_int;
      break;
    case BINARY_OPERATOR_GREATER_THAN:
      result.value_int = lhs.value_int > rhs.value_int;
      break;
    case BINARY_OPERATOR_LESS_THAN_EQUAL_TO:
      result.value_int = lhs.value_int <= rhs.value_int;
      break;
    case BINARY_OPERATOR_GREATER_THAN_EQUAL_TO:
      result.value_int = lhs.value_int >= rhs.value_int;
      break;
    case BINARY_OPERATOR_EQUALS:
      result.value_int = lhs.value_int == rhs.value_int;
      break;
    case BINARY_OPERATOR_NOT_EQUALS:
      result.value_int = lhs.value_int != rhs.value_int;
      break;
    case BINARY_OPERATOR_LOGICAL_AND:
      result.value_int = lhs.value_int && rhs.value_int;
      break;
    case BINARY_OPERATOR_LOGICAL_OR:
      result.value_int = lhs.value_int || rhs.value_int;
      break;
    default:
      UNREACHABLE;
  }

  return result;
}

static evaluate_value_t evaluate_function_call_expr(evaluate_context_t *ctx, ast_node_t *node)
{
  char *function_identifier = node->data_function_call_expr.token_callee->identifier;
  symbol_table_entry_t *symbol_table_function_entry =
    evaluate_env_lookup_function(ctx->current_env, function_identifier);

  VECTOR_T(evaluate_value_t) *argument_values = vector_create(sizeof(evaluate_value_t), 8);
  VECTOR_T(ast_node_t*) *argument_nodes = node->data_function_call_expr.arguments;
  for (int i = 0; i < argument_nodes->length; i++)
  {
    evaluate_value_t argument_value = evaluate_expr(ctx, VECTOR_ARR(argument_nodes, ast_node_t*)[i]);
    evaluate_value_t *argument_value_elt = vector_next_element(argument_values);
    memcpy(argument_value_elt, &argument_value, sizeof(evaluate_value_t));
  }

  evaluate_value_t returned_value =
    call_function(ctx,symbol_table_function_entry->function_node, argument_values);

  free(argument_values);
  return returned_value;
}

static evaluate_stmt_result_t evaluate_stmt_list(evaluate_context_t *ctx, ast_node_t *node)
{
  ast_node_t **stmts_arr = VECTOR_ARR(node->data_stmt_list.stmts, ast_node_t*);

  evaluate_context_enter_new_env(ctx);
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
    symbol_table_entry->variable_value = (evaluate_value_t){
      .data_type = DATA_TYPE_INT,
      .value_int = 0,
    };
  }
  else
    symbol_table_entry->variable_value = evaluate_expr(ctx, node->data_variable_decl.initializer);

  evaluate_env_add_symbol_table_entry(ctx->current_env, symbol_table_entry);
}

static void evaluate_function_decl(evaluate_context_t *ctx, ast_node_t *node)
{
  symbol_table_entry_t *symbol_table_entry = malloc(sizeof(symbol_table_entry_t));
  if (symbol_table_entry == NULL)
    exit_out_of_memory();

  symbol_table_entry->symbol_identifier = node->data_function_decl.token_identifier->identifier;
  symbol_table_entry->symbol_type = SYMBOL_TYPE_FUNCTION;
  symbol_table_entry->function_node = node;

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
  evaluate_value_t result = call_function(ctx, node_main_function_decl, arguments);
  vector_free(arguments);

  evaluate_context_free(ctx);
  return result;
}

evaluate_value_t evaluate(ast_node_t *node)
{
  return evaluate_program(node);
}
