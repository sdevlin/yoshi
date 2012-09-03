#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "env.h"
#include "builtin.h"
#include "err.h"
#include "read.h"
#include "print.h"
#include "interp.h"
#include "gc.h"

struct env global_env;

static FILE *infile;

int main(int argc, char **argv) {
  config_init(argc, argv);
  builtin_define(&global_env);
  infile = config_next_file();
  for (;;) {
    struct exp *e;
    if (infile == stdin) {
      printf("yoshi> ");
    }
    if (!err_init()) {
      if ((e = read(infile)) == NULL) {
        if ((infile = config_next_file()) == NULL) {
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
