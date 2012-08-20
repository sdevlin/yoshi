#include <stdlib.h>

#include "data.h"
#include "gc.h"

extern struct env global_env;

void gc_collect(void) {

}

struct exp *gc_alloc_exp(enum exp_type type) {
  return NULL;
}

struct env *gc_alloc_env(struct env *parent) {
  return NULL;
}
