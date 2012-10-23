#include <stdio.h>
#include <stdlib.h>

#include "input.h"
#include "file_input.h"

struct file_input {
  struct input impl;
  FILE *stream;
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

struct input *file_input_new(FILE *stream) {
  struct file_input *input = malloc(sizeof *input);
  input->impl = impl;
  input->stream = stream;
  return (struct input *)input;
}

static int get(struct input *self) {
  struct file_input *input = (struct file_input *)self;
  return getc(input->stream);
}

static void unget(struct input *self, int c) {
  struct file_input *input = (struct file_input *)self;
  ungetc(c, input->stream);
}

static int is_stdin(struct input *self) {
  struct file_input *input = (struct file_input *)self;
  return input->stream == stdin;
}

static void free_(struct input *self) {
  struct file_input *input = (struct file_input *)self;
  fclose(input->stream);
  free(input);
}
