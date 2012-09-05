#include <stdlib.h>

#include "exp.h"
#include "env.h"

void gc_collect(void) {

}

struct exp *gc_alloc_exp(enum exp_type type) {
  struct exp *e = calloc(1, sizeof *e);
  e->type = type;
  return e;
}

struct env *gc_alloc_env(struct env *parent) {
  struct env *e = calloc(1, sizeof *e);
  e->parent = parent;
  return e;
}
