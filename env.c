#include <stdlib.h>
#include <string.h>

#include "exp.h"
#include "env.h"
#include "err.h"

static struct exp *env_visit(struct env *env,
                             struct exp *symbol,
                             struct exp *value,
                             struct exp *(*found)(struct binding *b,
                                                  struct exp *v),
                             struct exp *(*not_found)(struct env *env,
                                                      struct exp *symbol,
                                                      struct exp *value)) {
  struct env *e = env;
  if (symbol->type != SYMBOL) {
    return err_error("env: expected symbol");
  }
  do {
    struct binding *b = e->bindings;
    while (b != NULL) {
      if (!strcmp(b->symbol, symbol->value.symbol)) {
        return (*found)(b, value);
      }
      b = b->next;
    }
    e = e->parent;
  } while (e != NULL);
  return (*not_found)(env, symbol, value);
}

static struct exp *error_not_found(struct env *env, struct exp *symbol,
                                   struct exp *value) {
  return err_error("env: no binding for symbol");
}

static struct exp *lookup_found(struct binding *binding, struct exp *_) {
  return binding->value;
}

struct exp *env_lookup(struct env *env, struct exp *symbol) {
  return env_visit(env, symbol, NULL, &lookup_found, &error_not_found);
}

static struct exp *update_found(struct binding *binding, struct exp *value) {
  binding->value = value;
  return OK;
}

struct exp *env_update(struct env *env, struct exp *symbol,
                              struct exp *value) {
  return env_visit(env, symbol, value, &update_found, &error_not_found);
}

static struct exp *define_not_found(struct env *env, struct exp *symbol,
                                    struct exp *value) {
  struct binding *b = malloc(sizeof *b);
  b->symbol = malloc(strlen(symbol->value.symbol) + 1);
  strcpy(b->symbol, symbol->value.symbol);
  b->value = value;
  b->next = env->bindings;
  env->bindings = b;
  return OK;
}

struct exp *env_define(struct env *env, struct exp *symbol,
                              struct exp *value) {
  return env_visit(env, symbol, value, &update_found, &define_not_found);
}
