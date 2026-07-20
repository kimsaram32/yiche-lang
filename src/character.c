#include "yiche.h"

int is_visible_character(char c)
{
  return c >= 33 && c <= 126;
}

int is_whitespace(char c)
{
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

int is_letter(char c)
{
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}

int is_digit(char c)
{
  return c >= '0' && c <= '9';
}

int is_character(char c)
{
  return is_visible_character(c) || is_whitespace(c);
}
