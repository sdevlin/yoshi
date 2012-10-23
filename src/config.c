#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "util/file_input.h"
#include "gc.h"

static struct {
  size_t count;
  char **names;
} file_info;

struct flags config;

struct gc *gc;

void config_init(int argc, char **argv) {
  argc -= 1;
  argv += 1;
  file_info.names = malloc(argc * (sizeof *file_info.names));
  while (argc > 0) {
    char *arg = *argv;
    if (!strcmp(arg, "-i")) {
      config.interactive = ON;
    } else if (!strcmp(arg, "-d")) {
      config.debug = ON;
    } else if (!strcmp(arg, "-s")) {
      config.silent = ON;
    } else {
      file_info.count += 1;
      file_info.names[file_info.count - 1] = arg;
    }
    argc -= 1;
    argv += 1;
  }
  if (file_info.count == 0) {
    config.interactive = ON;
  }
  gc = &gc_ms;
}

#ifndef PREFIX
#define PREFIX "."
#endif

struct input *config_next_input(void) {
  static size_t i = 0;
  static int stdlib_loaded = 0;
  static int yield_stdin = 1;
  if (!stdlib_loaded) {
    stdlib_loaded = 1;
    return file_input_new(fopen(PREFIX "/lib/yoshi/stdlib.scm", "r"));
  } else if (i < file_info.count) {
    FILE *f = fopen(*(file_info.names + i), "r");
    i += 1;
    return file_input_new(f);
  } else if (yield_stdin && config.interactive) {
    yield_stdin = 0;
    return file_input_new(stdin);
  } else {
    return NULL;
  }
}
