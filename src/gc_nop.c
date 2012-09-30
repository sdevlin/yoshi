#include <stdlib.h>

#include "exp.h"
#include "env.h"
#include "gc.h"

static void gc_init(void);
static void gc_collect(void);
static struct exp *gc_alloc_exp(enum exp_type type);
static struct env *gc_alloc_env(struct env *parent);

struct gc gc_nop = {
  .init = &gc_init,
  .collect = &gc_collect,
  .alloc_exp = &gc_alloc_exp,
  .alloc_env = &gc_alloc_env
};

static void gc_init(void) {

}

static void gc_collect(void) {

}

static struct exp *gc_alloc_exp(enum exp_type type) {
  struct exp *e = calloc(1, sizeof *e);
  e->type = type;
  return e;
}

static struct env *gc_alloc_env(struct env *parent) {
  struct env *e = calloc(1, sizeof *e);
  e->parent = parent;
  return e;
}
