#include <stdlib.h>

#include "exp.h"
#include "env.h"
#include "gc.h"
#include "util/vector.h"

static void gc_init(void);
static void gc_collect(void);
static struct exp *gc_alloc_exp(enum exp_type type);
static struct env *gc_alloc_env(struct env *parent);

struct gc gc_copy = {
  .init = &gc_init,
  .collect = &gc_collect,
  .alloc_exp = &gc_alloc_exp,
  .alloc_env = &gc_alloc_env
};

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
  struct record *fwd;
};

struct frame {
  size_t capacity;
  struct record *data;
  struct record *next;
  struct record *end;
};

static const size_t frame_min_size = 1024;

static struct frame *frame_new(size_t capacity) {
  struct frame *frame = malloc(sizeof *frame);
  frame->capacity = capacity > 0 ? capacity : frame_min_size;
  frame->data = calloc(frame->capacity, sizeof *frame->data);
  frame->next = frame->data;
  frame->end = frame->next + frame->capacity;
  return frame;
}

static void frame_trim(struct frame *frame) {
  size_t capacity = (frame->next - frame->data) * 10;
  capacity = capacity > frame_min_size ? capacity : frame_min_size;
  /* log if capacity < frame->capacity */
  frame->capacity = capacity;
  frame->data = realloc(frame->data, frame->capacity * sizeof *frame->data);
}

static void frame_free(struct frame *frame) {
  free(frame->data);
  free(frame);
}

struct {
  struct frame *root;
  struct frame *swap;
} gc;

void gc_init(void) {
  gc.root = frame_new(0);
}

static struct exp *gc_copy_exp(struct exp *exp);
static struct env *gc_copy_env(struct env *env);

void gc_collect(void) {

}

static void _gc_collect(void) {
  gc.swap = frame_new(gc.root->capacity * 10);
  gc_copy_env(&global_env);
  frame_trim(gc.swap);
  frame_free(gc.root);
  gc.root = gc.swap;
  gc.swap = NULL;
}
#include <stdio.h>
static void *gc_alloc(enum record_type type) {
  if (gc.root->next == gc.root->end) {
    printf("starting collection... ");
    _gc_collect();
    printf("done\n");
  }
  struct record *rec = gc.root->next;
  gc.root->next += 1;
  rec->type = type;
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

#define IS_NOT(x) (ptr != (x))

static int gc_is_managed(void *ptr) {
  return IS_NOT(&global_env) &&
    IS_NOT(OK) &&
    IS_NOT(NIL) &&
    IS_NOT(TRUE) &&
    IS_NOT(FALSE);
}

#undef IS_NOT

#define COPY(x, type) (x = gc_copy_##type(x))

#define MAYBE_COPY(type)                                \
  do {                                                  \
    if (gc_is_managed(type)) {                          \
      struct record *rec = (struct record *)type;       \
      if (rec->fwd == NULL) {                           \
        *gc.swap->next = *rec;                          \
        rec->fwd = gc.swap->next;                       \
        type = (struct type *)gc.swap->next;            \
        gc.swap->next += 1;                             \
      } else {                                          \
        return (struct type *)rec->fwd;                 \
      }                                                 \
    }                                                   \
  } while (0)

static struct exp *gc_copy_exp(struct exp *exp) {
  MAYBE_COPY(exp);
  switch (exp->type) {
  case PAIR:
    COPY(exp->value.pair.first, exp);
    COPY(exp->value.pair.rest, exp);
    break;
  case VECTOR:
    {
      size_t len = vector_length(exp->value.vector);
      size_t i;
      for (i = 0; i < len; i += 1) {
        struct exp *item = vector_get(exp->value.vector, i);
        vector_put(exp->value.vector, i, gc_copy_exp(item));
      }
    }
    break;
  case CLOSURE:
    COPY(exp->value.closure.params, exp);
    COPY(exp->value.closure.body, exp);
    COPY(exp->value.closure.env, env);
    break;
  default:
    break;
  }
  return exp;
}

/* it would be nice if this only traversed global_env once */
static struct env *gc_copy_env(struct env *env) {
  MAYBE_COPY(env);
  struct binding *b = env->bindings;
  while (b != NULL) {
    COPY(b->value, exp);
    b = b->next;
  }
  if (env->parent != NULL) {
    COPY(env->parent, env);
  }
  return env;
}

#undef MAYBE_COPY

#undef COPY
