#ifndef YICHE_COMMON_H
#define YICHE_COMMON_H

#define ARRAY_LENGTH(arr) (sizeof(arr) / sizeof((arr)[0]))

#define UNREACHABLE __builtin_unreachable()

#endif

#include "error.h"
#include "vector.h"
#include "character.h"
#include "input.h"
#include "ast.h"
#include "tokenizer.h"
#include "parser.h"
