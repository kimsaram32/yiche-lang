#ifndef YICHE_COMMON_H
#define YICHE_COMMON_H

#define ARRAY_LENGTH(arr) (sizeof(arr) / sizeof((arr)[0]))

#define UNREACHABLE __builtin_unreachable()
#define NORETURN __attribute__((__noreturn__))

typedef void (*destructor_t)(void *p);

void destructor_noop(void *p);

#endif
