#include <stdlib.h>

#include "data.h"
#include "gc.h"

extern struct env global_env;

static struct {
  struct {
    struct exp root;
    size_t count;
  } exp;
  struct {
    struct env root;
    size_t count;
  } env;
} gc;

static void gc_mark_env(struct env *env);
static void gc_sweep(void);
static void gc_free_exp(struct exp *exp);
static void gc_free_env(struct env *env);

void gc_collect(void) {
  gc_mark_env(&global_env);
  gc_sweep();
  // TODO
  /*
  if (config.flags.debug) {
    printf("gc: %d exps %d envs\n", gc.exp.count, gc.env.count);
  }
  */
}

static void gc_mark_exp(struct exp *exp) {
  if (exp->mark) {
    return;
  }
  exp->mark = KEEP;
  switch (exp->type) {
  case PAIR:
    gc_mark_exp(exp->value.pair.first);
    gc_mark_exp(exp->value.pair.rest);
    break;
  case CLOSURE:
    gc_mark_exp(exp->value.closure.params);
    gc_mark_exp(exp->value.closure.body);
    gc_mark_env(exp->value.closure.env);
    break;
  default:
    break;
  }
}

static void gc_mark_env(struct env *env) {
  if (env != &global_env && env->mark == KEEP) {
    return;
  }
  env->mark = KEEP;
  struct binding *b = env->bindings;
  while (b != NULL) {
    gc_mark_exp(b->value);
    b = b->next;
  }
  if (env->parent != NULL) {
    gc_mark_env(env->parent);
  }
}

static void gc_sweep(void) {
#define SWEEP(type)                              \
  do {                                           \
    struct type *prev = &gc.type.root;           \
    struct type *curr = prev->next;              \
    while (curr != NULL) {                       \
      if (curr->mark) {                          \
        curr->mark = FREE;                       \
        prev = curr;                             \
        curr = curr->next;                       \
      } else {                                   \
        prev->next = curr->next;                 \
        gc_free_ ## type(curr);                  \
        curr = prev->next;                       \
      }                                          \
    }                                            \
  } while (0)
  SWEEP(exp);
  SWEEP(env);
#undef SWEEP
}

#define MANAGE(type)                            \
  do {                                          \
    e->mark = FREE;                             \
    e->next = gc.type.root.next;                \
    gc.type.root.next = e;                      \
    gc.type.count += 1;                         \
  } while (0)

struct exp *gc_alloc_exp(enum exp_type type) {
  struct exp *e = malloc(sizeof *e);
  e->type = type;
  MANAGE(exp);
  return e;
}

struct env *gc_alloc_env(struct env *parent) {
  struct env *e = malloc(sizeof *e);
  e->bindings = NULL;
  e->parent = parent;
  MANAGE(env);
  return e;
}

#undef MANAGE

static void gc_free_exp(struct exp *exp) {
  switch (exp->type) {
  case SYMBOL:
    free(exp->value.symbol);
    break;
  default:
    break;
  }
  free(exp);
  gc.exp.count -= 1;
}

static void gc_free_env(struct env *env) {
  struct binding *prev = NULL;
  struct binding *curr = env->bindings;
  while (curr != NULL) {
    free(curr->symbol);
    prev = curr;
    curr = curr->next;
    free(prev);
  }
  free(env);
  gc.env.count -= 1;
}
