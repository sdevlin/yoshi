#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "exp.h"
#include "env.h"
#include "err.h"
#include "gc.h"

static int is_self_eval(struct exp *exp);
static int is_var(struct exp *exp);
static int is_apply(struct exp *exp);
static struct exp *map_eval(struct exp *exp, void *data);
static struct env *extend_env(struct exp *params, struct exp *args,
                              struct env *parent);

struct exp *eval(struct exp *exp, struct env *env) {
  for (;;) {
    if (config.debug) {
      char *str = exp_stringify(exp);
      printf("eval: %s\n", str);
      free(str);
    }
    if (is_self_eval(exp)) {
      return exp;
    } else if (is_var(exp)) {
      return env_lookup(env, exp);
    } else if (exp_list_tagged(exp, "quote")) {
      return CADR(exp);
    } else if (exp_list_tagged(exp, "set!")) {
      return env_update(env, CADR(exp), eval(CADDR(exp), env));
    } else if (exp_list_tagged(exp, "define")) {
      struct exp *id = CADR(exp);
      struct exp *value = eval(CADDR(exp), env);
      if (IS(value, CLOSURE) && value->value.closure.name == NULL) {
        value->value.closure.name = malloc(strlen(id->value.symbol) + 1);
        strcpy(value->value.closure.name, id->value.symbol);
      }
      return env_define(env, id, value);
    } else if (exp_list_tagged(exp, "if")) {
      if (eval(CADR(exp), env) != FALSE) {
        exp = CADDR(exp);
      } else {
        exp = CADDDR(exp);
      }
    } else if (exp_list_tagged(exp, "or")) { /* need to get rid of this */
      exp = CDR(exp);
      if (exp == NIL) {
        return FALSE;
      } else {
        while (CDR(exp) != NIL) {
          struct exp *result = eval(CAR(exp), env);
          if (result != FALSE) {
            return result;
          }
          exp = CDR(exp);
        }
        exp = CAR(exp);
      }
    } else if (exp_list_tagged(exp, "lambda")) {
      return exp_make_closure(CADR(exp), CADDR(exp), env);
    } else if (exp_list_tagged(exp, "begin")) {
      exp = CDR(exp);
      if (exp == NIL) {
        return OK;
      } else {
        while (CDR(exp) != NIL) {
          eval(CAR(exp), env);
          exp = CDR(exp);
        }
        exp = CAR(exp);
      }
    } else if (is_apply(exp)) {
      struct exp *fn;
      struct exp *args;
      exp = exp_list_map(exp, &map_eval, env);
      fn = CAR(exp);
      args = CDR(exp);
      switch (fn->type) {
      case FUNCTION:
        return (*fn->value.function.fn)(args);
      case CLOSURE:
        env = extend_env(fn->value.closure.params, args,
                         fn->value.closure.env);
        exp = fn->value.closure.body;
        break;
      default:
        return err_error("eval: bad function type");
      }
    } else {
      return err_error("eval: unknown exp type");
    }
  }
}

static int is_self_eval(struct exp *exp) {
  return (IS(exp, UNDEFINED) ||
          IS(exp, FIXNUM) ||
          IS(exp, STRING) ||
          IS(exp, CHARACTER) ||
          IS(exp, BOOLEAN) ||
          IS(exp, BYTEVECTOR));
}

static int is_var(struct exp *exp) {
  return IS(exp, SYMBOL);
}

static int is_apply(struct exp *exp) {
  return IS(exp, PAIR);
}

static struct exp *map_eval(struct exp *exp, void *data) {
  struct env *env = data;
  return eval(exp, env);
}

static struct env *extend_env(struct exp *params, struct exp *args,
                              struct env *parent) {
  struct env *env = (*gc->alloc_env)(parent);
  for (;;) {
    if (params == NIL && args == NIL) {
      return env;
    } else if (params == NIL) {
      return err_error("apply: too many args");
    } else if (IS(params, PAIR)) {
      if (args == NIL) {
        return err_error("apply: too few args");
      }
      /* FIX non-symbol params should be an error */
      if (IS(CAR(params), SYMBOL)) {
        env_define(env, CAR(params), CAR(args));
      }
      params = CDR(params);
      args = CDR(args);
    } else if (IS(params, SYMBOL)) {
      env_define(env, params, args);
      return env;
    }
  }
}
