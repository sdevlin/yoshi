#include <stdlib.h>

#include "exp.h"
#include "env.h"

enum record_type {
  EXP,
  ENV
};

enum mark_type {
  FREE,
  KEEP
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

void gc_collect(void) {
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
      curr->mark = FREE;
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

struct exp *gc_alloc_exp(enum exp_type type) {
  struct exp *e = gc_alloc(EXP);
  e->type = type;
  return e;
}

struct env *gc_alloc_env(struct env *parent) {
  struct env *e = gc_alloc(ENV);
  e->parent = parent;
  return e;
}

static void gc_free(struct record *rec) {
  switch (rec->type) {
  case EXP:
    if (rec->data.exp.type == SYMBOL) {
      free(rec->data.exp.value.symbol);
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

#define IS_NOT(x) (ptr != (x))

static int gc_is_managed(void *ptr) {
  return IS_NOT(&global_env) &&
    IS_NOT(OK) &&
    IS_NOT(NIL) &&
    IS_NOT(TRUE) &&
    IS_NOT(FALSE);
}

static void gc_maybe_mark(void *ptr) {
  if (gc_is_managed(ptr)) {
    struct record *rec = ptr;
    rec->mark = KEEP;
  }
}

static int gc_should_proceed(void *ptr) {
  if (gc_is_managed(ptr)) {
    struct record *rec = ptr;
    return rec->mark == FREE;
  } else {
    return ptr == &global_env;
  }
}

#undef IS_NOT
