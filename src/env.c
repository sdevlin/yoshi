#include <stdlib.h>
#include <string.h>

#include "exp.h"
#include "env.h"
#include "err.h"

#define FOREACH_ENV(code)                       \
  do {                                          \
    { code; }                                   \
    env = env->parent;                          \
  } while (env != NULL);
#define FOREACH_BINDING(code)                   \
  struct binding *b = env->bindings;            \
  while (b != NULL) {                           \
    { code; }                                   \
    b = b->next;                                \
  }
#define IF_FOUND(code)                                  \
  if (!strcmp(b->symbol, symbol->value.symbol)) {       \
    code;                                               \
  }

struct exp *env_lookup(struct env *env, struct exp *symbol) {
  err_ensure(IS(symbol, SYMBOL), "env: expected symbol");
  FOREACH_ENV({
      FOREACH_BINDING({
          IF_FOUND({
              return b->value;
            });
        });
    });
  return err_error("env: no binding for symbol");
}

struct exp *env_update(struct env *env, struct exp *symbol,
                       struct exp *value) {
  err_ensure(IS(symbol, SYMBOL), "env: expected symbol");
  FOREACH_ENV({
      FOREACH_BINDING({
          IF_FOUND({
              b->value = value;
              return OK;
            });
        });
    });
  return err_error("env: no binding for symbol");
}

struct exp *env_define(struct env *env, struct exp *symbol,
                       struct exp *value) {
  err_ensure(IS(symbol, SYMBOL), "env: expected symbol");
  FOREACH_BINDING({
      IF_FOUND({
          b->value = value;
          return OK;
        });
    });
  b = malloc(sizeof *b);
  b->symbol = malloc(strlen(symbol->value.symbol) + 1);
  strcpy(b->symbol, symbol->value.symbol);
  b->value = value;
  b->next = env->bindings;
  env->bindings = b;
  return OK;
}
