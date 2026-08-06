#ifndef YICHE_PARSER_H
#define YICHE_PARSER_H

#include "ast.h"
#include "tokenizer.h"

typedef enum
{
  PARSER_SUCCESS = 0,
  PARSER_OOM,

  PARSER_UNEXPECTED_TOKEN,
  PARSER_OUT_OF_TOKENS,
}
parser_result_t;

// Follows the same naming convention as 'tokenizer_context_t'.
typedef struct parser_context_t parser_context_t;

// 'tokenizer' is not owned by the parser and therefore need to be maanged separately.
parser_context_t *parser_context_create(tokenizer_context_t *tokenizer);

void parser_context_free(parser_context_t *ctx);

// Print 'result', an error, to stdout. If 'result' is not an error code, do
// nothing. NOTE: Will be moved once the test code does not depend on the
// formatting of error messages.
void parser_print_error(parser_context_t *parser, parser_result_t result);

// Parse the tokens supplied by 'tokenizer'. '*node_p' must be NULL.
//
// On success, all tokens in 'tokenizer' are consumed, and '*node_p' is set to
// the pointer of the root program node (i.e., node of type 'AST_NODE_PROGRAM').
//
// On failure, tokens in 'tokenizer' are partially consumed, and '*node_p' is
// set to NULL.
parser_result_t parser_parse(parser_context_t *tokenizer, ast_node_t **node_p);

#endif
