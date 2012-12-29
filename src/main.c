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
#include "util/input.h"
#include "print.h"
#include "gc.h"

struct env global_env;

static struct input *input;

int main(int argc, char **argv) {
  config_init(argc, argv);
  (*gc->init)();
  builtin_defall(&global_env);
  input = config_next_input();
  for (;;) {
    struct exp *e;
    if (input->is_stdin(input)) {
      printf("yoshi> ");
    }
    if (!err_init()) {
      if ((e = read(input)) == NULL) {
        if ((input = config_next_input()) == NULL) {
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
      char *msg = err_message();
      printf("error: %s\n", msg);
      free(msg);
    }
    (*gc->collect)();
  }
}
