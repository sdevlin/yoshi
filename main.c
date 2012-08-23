#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "flag.h"
#include "env.h"
#include "err.h"
#include "gc.h"
#include "interp.h"
#include "read.h"
#include "builtin.h"
#include "print.h"
#include "strbuf.h"

struct env global_env;

static struct {
  struct {
    size_t count;
    char **names;
  } files;
  struct {
    enum flag_type interactive;
    enum flag_type debug;
  } flags;
} config;
static FILE *infile;
static FILE *next_file(void);

static void parse_args(int argc, char **argv);

int main(int argc, char **argv) {
  parse_args(argc, argv);
  builtin_define(&global_env);
  infile = next_file();
  for (;;) {
    struct exp *e;
    if (infile == stdin) {
      printf("yoshi> ");
    }
    if (!err_init()) {
      if ((e = read(infile)) == NULL) {
        if ((infile = next_file()) == NULL) {
          return 0;
        } else {
          continue;
        }
      }
      print(eval(e, &global_env));
    } else {
      printf("error: %s\n", err_msg);
    }
    gc_collect();
  }
}

static void parse_args(int argc, char **argv) {
  argc -= 1;
  argv += 1;
  config.files.names = malloc(argc * (sizeof *config.files.names));
  while (argc > 0) {
    char *arg = *argv;
    if (!strcmp(arg, "-i")) {
      config.flags.interactive = ON;
    } else if (!strcmp(arg, "-d")) {
      config.flags.debug = ON;
    } else {
      config.files.count += 1;
      config.files.names[config.files.count - 1] = arg;
    }
    argc -= 1;
    argv += 1;
  }
  if (config.files.count == 0) {
    config.flags.interactive = ON;
  }
}

static FILE *next_file(void) {
  static size_t i = 0;
  if (infile == NULL) {
    return fopen("/home/sean/proj/yoshi/lib/libyoshi.scm", "r");
  } else if (i < config.files.count) {
    FILE *f = fopen(*(config.files.names + i), "r");
    i += 1;
    return f;
  } else if (infile != stdin && config.flags.interactive) {
    return stdin;
  } else {
    return NULL;
  }
}
