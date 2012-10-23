#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "input.h"
#include "str_input.h"

struct str_input {
  struct input impl;
  char *str;
  size_t cursor;
};

static int get(struct input *self);
static void unget(struct input *self, int c);
static int is_stdin(struct input *self);
static void free_(struct input *self);

static struct input impl = {
  .get = &get,
  .unget = &unget,
  .is_stdin = &is_stdin,
  .free = &free_
};

struct input *str_input_new(const char *str) {
  struct str_input *input = malloc(sizeof *input);
  input->impl = impl;
  input->str = malloc(strlen(str) + 1);
  strcpy(input->str, str);
  input->cursor = 0;
  return (struct input *)input;
}

static int get(struct input *self) {
  struct str_input *input = (struct str_input *)self;
  int c = *(input->str + input->cursor);
  if (c != '\0') {
    input->cursor += 1;
    return c;
  } else {
    return EOF;
  }
}

static void unget(struct input *self, int c) {
  struct str_input *input = (struct str_input *)self;
  if (input->cursor > 0) {
    input->cursor -= 1;
    *(input->str + input->cursor) = c;
  }
}

static int is_stdin(struct input *self) {
  return 0;
}

static void free_(struct input *self) {
  struct str_input *input = (struct str_input *)self;
  free(input->str);
  free(input);
}
