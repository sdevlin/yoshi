#include <stdlib.h>

#include "data.h"
#include "gc.h"

extern struct env global_env;

enum record_type {
  EXP,
  ENV
};

struct record {
  union {
    struct exp exp;
    struct env env;
  } data;
  enum record_type type;
  enum flag_type mark;
  struct record *next;
};

static struct record root;


static void gc_mark_exp(struct exp *exp);
static void gc_mark_env(struct env *env);
static void gc_sweep(void);
static void gc_free(struct record *rec);

void gc_collect(void) {
  gc_mark_env(&global_env);
  gc_sweep();
}

static void gc_mark_exp(struct exp *exp) {
  // this check is ugly, need a better way to filter the constants
  // with a copying gc, it would be easy just to check if the pointer
  // is in the bounds of managed memory
  if (exp != OK && exp != NIL && exp != TRUE && exp != FALSE) {
    struct record *rec = (struct record *)exp;
    if (rec->mark) {
      return;
    } else {
      rec->mark = ON;
    }
  }
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
  if (env != &global_env) {
    struct record *rec = (struct record *)env;
    if (rec->mark) {
      return;
    } else {
      rec->mark = ON;
    }
  }
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
      curr->mark = OFF;
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
