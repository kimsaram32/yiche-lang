#ifndef YICHE_COMMON_H
#define YICHE_COMMON_H

#if defined(__GNUC__) || defined(__clang__)
  #define UNREACHABLE (__builtin_unreachable())
#elif defined(_MSC_VER)
  #define UNREACHABLE (__assume(0))
#endif

#if defined(__GNUC__) || defined(__clang__)
  #define NORETURN __attribute__((__noreturn__))
#elif defined(_MSC_VER)
  #define NORETURN __declspec(noreturn)
#endif

#define ARRAY_LENGTH(arr) (sizeof(arr) / sizeof((arr)[0]))

typedef void (*destructor_t)(void *p);

void destructor_noop(void *p);

#endif
