#ifndef _INPUT_H
#define _INPUT_H

typedef struct
{
  unsigned char c; // 0 is used for marking EOF.
  int line, column;
}
input_char_t;

void input_init(void);

// 2026-07-21 The function 'input_get_last_char' could be removed by making
// 'input_advance_char' and 'input_peek_char' return the full input_char_t
// struct. But most callers only need the raw character, and writing .c
// everywhere would make the code verbose. Therefore, I separated the function for
// retrieving the full information.

input_char_t input_get_last_char(void);
unsigned char input_advance_char(void);
unsigned char input_peek_char(int n);

// the buffer changes as input_advance_char() is called. return the
// accumulated string and clear the buffer.
char *input_get_and_clear_buffer(void);

#endif
