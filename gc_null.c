#include <stdlib.h>

#include "exp.h"
#include "env.h"

void gc_collect(void) {

}

struct exp *gc_alloc_exp(enum exp_type type) {
  struct exp *e = calloc(sizeof *e, 1);
  e->type = type;
  return e;
}

struct env *gc_alloc_env(struct env *parent) {
  struct env *e = calloc(sizeof *e, 1);
  e->parent = parent;
  return e;
}
