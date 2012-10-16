#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "env.h"
#include "builtin.h"
#include "err.h"
#include "read.h"
#include "expand.h"
#include "eval.h"
#include "print.h"
#include "gc.h"

struct env global_env;

static FILE *input;

int main(int argc, char **argv) {
  config_init(argc, argv);
  (*gc->init)();
  builtin_define(&global_env);
  input = config_next_file();
  for (;;) {
    struct exp *e;
    if (input == stdin) {
      printf("yoshi> ");
    }
    if (!err_init()) {
      if ((e = read(input)) == NULL) {
        if ((input = config_next_file()) == NULL) {
          return 0;
        } else {
          continue;
        }
      }
      e = eval(expand(e), &global_env);
      if (!config.silent) {
        print(e);
      }
    } else {
      printf("error: %s\n", err_msg);
    }
    (*gc->collect)();
  }
}
