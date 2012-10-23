#ifndef FILE_INPUT_H
#define FILE_INPUT_H
#include <stdio.h>
#include "input.h"
extern struct input *file_input_new(FILE *stream);
struct file_input;
extern void file_input_free(struct file_input *input);
#endif
