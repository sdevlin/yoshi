#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "exp.h"
#include "env.h"
#include "err.h"
#include "gc.h"

static int is_self_eval(struct exp *exp);
static int is_var(struct exp *exp);
static int is_apply(struct exp *exp);
static struct exp *expand_quasiquote(struct exp *exp);
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
    } else if (exp_list_tagged(exp, "quasiquote")) {
      exp = expand_quasiquote(CADR(exp));
    } else if (exp_list_tagged(exp, "set!")) {
      return env_update(env, CADR(exp), eval(CADDR(exp), env));
    } else if (exp_list_tagged(exp, "define")) {
      struct exp *id = CADR(exp);
      return env_define(env, id, eval(CADDR(exp), env));
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
        return (*fn->value.function)(args);
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

/* doesn't handle nested quasiquote */
static struct exp *expand_quasiquote(struct exp *exp) {
  if (!IS(exp, PAIR)) {
    return exp_make_list(exp_make_symbol("quote"),
                         exp,
                         NULL);
  }
  err_ensure(!exp_list_tagged(exp, "unquote-splicing"),
             "expand_quasiquote: unexpected unquote-splicing");
  if (exp_list_tagged(exp, "unquote")) {
    err_ensure(exp_list_length(exp) == 2,
               "expand_quasiquote: bad syntax in unquote");
    return CADR(exp);
  } else if (exp_list_tagged(CAR(exp), "unquote-splicing")) {
    err_ensure(exp_list_length(CAR(exp)) == 2,
               "expand_quasiquote: bad syntax in unquote-splicing");
    return exp_make_list(exp_make_symbol("append"),
                         CADR(CAR(exp)),
                         expand_quasiquote(CDR(exp)),
                         NULL);
  } else {
      return exp_make_list(exp_make_symbol("cons"),
                           expand_quasiquote(CAR(exp)),
                           expand_quasiquote(CDR(exp)),
                           NULL);
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
