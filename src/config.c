#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

static struct {
  size_t count;
  char **names;
} file_info;

struct flags config;

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
}

#ifndef PREFIX
#define PREFIX "/home/sean/proj/yoshi/"
#endif

FILE *config_next_file(void) {
  static size_t i = 0;
  static int stdlib_loaded = 0;
  static int yield_stdin = 1;
  if (!stdlib_loaded) {
    stdlib_loaded = 1;
    return fopen(PREFIX "/lib/yoshi/stdlib.scm", "r");
  } else if (i < file_info.count) {
    FILE *f = fopen(*(file_info.names + i), "r");
    i += 1;
    return f;
  } else if (yield_stdin && config.interactive) {
    yield_stdin = 0;
    return stdin;
  } else {
    return NULL;
  }
}
