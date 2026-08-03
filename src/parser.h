#ifndef YICHE_PARSER_H
#define YICHE_PARSER_H

#include "ast.h"
#include "tokenizer.h"


ast_node_t *parse(tokenizer_context_t *tokenizer);

#endif
