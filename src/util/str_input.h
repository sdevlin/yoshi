#ifndef STR_INPUT_H
#define STR_INPUT_H
#include "input.h"
extern struct input *str_input_new(const char *str);
struct str_input;
extern void str_input_free(struct str_input *input);
#endif
