#include <stdlib.h>

#include "exp.h"
#include "gc.h"
#include "gc_alloc.h"

void gc_collect(void) {

}

struct exp *gc_alloc_exp(enum exp_type type) {
  return NULL;
}

struct env *gc_alloc_env(struct env *parent) {
  return NULL;
}
