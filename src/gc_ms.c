#include <stdlib.h>

#include "exp.h"
#include "env.h"
#include "gc.h"
#include "util/vector.h"

static void gc_init(void);
static void gc_collect(void);
static struct exp *gc_alloc_exp(enum exp_type type);
static struct env *gc_alloc_env(struct env *parent);

struct gc gc_ms = {
  .init = &gc_init,
  .collect = &gc_collect,
  .alloc_exp = &gc_alloc_exp,
  .alloc_env = &gc_alloc_env
};

enum record_type {
  EXP,
  ENV
};

enum mark_type {
  WHITE,
  BLACK
};

struct record {
  union {
    struct exp exp;
    struct env env;
  } data;
  enum record_type type;
  enum mark_type mark;
  struct record *next;
};

static struct record root;

static void gc_maybe_mark(void *ptr);
static int gc_should_proceed(void *ptr);
static void gc_mark_exp(struct exp *exp);
static void gc_mark_env(struct env *env);
static void gc_sweep(void);
static void gc_free(struct record *rec);

static void gc_init(void) {

}

static void gc_collect(void) {
  gc_mark_env(&global_env);
  gc_sweep();
}

static void gc_mark_exp(struct exp *exp) {
  if (!gc_should_proceed(exp)) {
    return;
  }
  gc_maybe_mark(exp);
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
  if (!gc_should_proceed(env)) {
    return;
  }
  gc_maybe_mark(env);
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
  struct record *prev = &root;
  struct record *curr = prev->next;
  while (curr != NULL) {
    if (curr->mark) {
      curr->mark = WHITE;
      prev = curr;
      curr = curr->next;
    } else {
      prev->next = curr->next;
      gc_free(curr);
      curr = prev->next;
    }
  }
}

static void *gc_alloc(enum record_type type) {
  struct record *rec = calloc(1, sizeof *rec);
  rec->type = type;
  rec->next = root.next;
  root.next = rec;
  return rec;
}

static struct exp *gc_alloc_exp(enum exp_type type) {
  struct exp *e = gc_alloc(EXP);
  e->type = type;
  return e;
}

static struct env *gc_alloc_env(struct env *parent) {
  struct env *e = gc_alloc(ENV);
  e->parent = parent;
  return e;
}

static void gc_free(struct record *rec) {
  switch (rec->type) {
  case EXP:
    switch (rec->data.exp.type) {
    case SYMBOL:
      free(rec->data.exp.value.symbol);
      break;
    case STRING:
      free(rec->data.exp.value.string);
      break;
    case VECTOR:
      vector_free(&rec->data.exp.value.vector, NULL);
      break;
    default:
      break;
    }
    break;
  case ENV:
    {
      struct binding *prev = NULL;
      struct binding *curr = rec->data.env.bindings;
      while (curr != NULL) {
        free(curr->symbol);
        prev = curr;
        curr = curr->next;
        free(prev);
      }
    }
    break;
  default:
    // error
    break;
  }
  free(rec);
}

static int gc_is_managed(void *ptr) {
#define IS_NOT(x) (ptr != (x))
  return IS_NOT(&global_env) &&
    IS_NOT(OK) &&
    IS_NOT(NIL) &&
    IS_NOT(TRUE) &&
    IS_NOT(FALSE);
#undef IS_NOT
}

static void gc_maybe_mark(void *ptr) {
  if (gc_is_managed(ptr)) {
    struct record *rec = ptr;
    rec->mark = BLACK;
  }
}

static int gc_should_proceed(void *ptr) {
  if (gc_is_managed(ptr)) {
    struct record *rec = ptr;
    return rec->mark == WHITE;
  } else {
    return ptr == &global_env;
  }
}
