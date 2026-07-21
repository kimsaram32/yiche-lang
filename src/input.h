#ifndef _INPUT_H
#define _INPUT_H

typedef struct
{
  unsigned char c;
  int line, column;
}
input_char_t;

// c == 0 if EOF is reached.
input_char_t input_get_last_read_char(void);

// returns 0 if EOF is reached.
unsigned char input_get_char(void);

void input_unget_char(void);

// the read buffer changes as input_get_char()/input_unget_char() are called. return the
// accumulated string and clear the buffer.
char *input_get_and_clear_read_buffer(void);

#endif
