#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "exp.h"
#include "env.h"
#include "err.h"
#include "gc_alloc.h"

static int is_self_eval(struct exp *exp);
static int is_var(struct exp *exp);
static int is_tagged(struct exp *exp, const char *s);
static int is_apply(struct exp *exp);
static struct exp *eval_and(struct exp *and, struct env *env);
static struct exp *eval_or(struct exp *or, struct env *env);
static struct exp *expand_quasiquote(struct exp *exp);
static struct exp *expand_begin(struct exp *forms);
static struct exp *expand_cond(struct exp *cond);
static struct env *extend_env(struct exp *params, struct exp *args,
                              struct env *parent);
static struct exp *map_list(struct exp *list, struct env *env,
                            struct exp *(*fn)(struct exp *exp,
                                              struct env *env));

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
    } else if (is_tagged(exp, "quote")) {
      err_ensure(exp_list_length(exp) == 2,
                 "eval: bad syntax in quote");
      return exp_nth(exp, 1);
    } else if (is_tagged(exp, "quasiquote")) {
      exp = expand_quasiquote(exp_nth(exp, 1));
    } else if (is_tagged(exp, "set!")) {
      return env_update(env, exp_nth(exp, 1), eval(exp_nth(exp, 2), env));
    } else if (is_tagged(exp, "define")) {
      struct exp *id = CAR(CDR(exp));
      switch (id->type) {
      case SYMBOL:
        return env_define(env, exp_nth(exp, 1), eval(exp_nth(exp, 2), env));
      case PAIR:
        return env_define(env, CAR(id), exp_make_closure(CDR(id),
                                                         CDR(CDR(exp)),
                                                         env));
      default:
        return err_error("define: bad syntax");
      }
    } else if (is_tagged(exp, "if")) {
      if (eval(exp_nth(exp, 1), env) != FALSE) {
        exp = exp_nth(exp, 2);
      } else {
        exp = exp_nth(exp, 3);
      }
    } else if (is_tagged(exp, "and")) {
      return eval_and(CDR(exp), env);
    } else if (is_tagged(exp, "or")) {
      return eval_or(CDR(exp), env);
    } else if (is_tagged(exp, "lambda")) {
      return exp_make_closure(exp_nth(exp, 1), CDR(CDR(exp)), env);
    } else if (is_tagged(exp, "begin")) {
      exp = expand_begin(CDR(exp));
    } else if (is_tagged(exp, "cond")) {
      exp = expand_cond(CDR(exp));
    } else if (is_apply(exp)) {
      struct exp *fn;
      struct exp *args;
      exp = map_list(exp, env, &eval);
      fn = CAR(exp);
      args = CDR(exp);
      switch (fn->type) {
      case FUNCTION:
        return (*fn->value.function)(args);
      case CLOSURE:
        env = extend_env(fn->value.closure.params, args,
                         fn->value.closure.env);
        exp = fn->value.closure.body;
        if (exp == NIL) {
          return OK;
        } else {
          while (CDR(exp) != NIL) {
            eval(CAR(exp), env);
            exp = CDR(exp);
          }
          exp = CAR(exp);
        }
        break;
      default:
        return err_error("eval: bad function type");
      }
    } else {
      return err_error("eval: unknown exp type");
    }
  }
}

static struct exp *map_list(struct exp *list, struct env *env,
                            struct exp *(*fn)(struct exp *exp,
                                              struct env *env)) {
  if (list == NIL) {
    return NIL;
  } else {
    struct exp *first = (*fn)(CAR(list), env);
    struct exp *rest = map_list(CDR(list), env, fn);
    return exp_make_pair(first, rest);
  }
}

static struct exp *eval_and(struct exp *and, struct env *env) {
  struct exp *result = TRUE;
  while (and != NIL) {
    result = eval(CAR(and), env);
    if (result == FALSE) {
      return FALSE;
    }
    and = CDR(and);
  }
  return result;
}

static struct exp *eval_or(struct exp *or, struct env *env) {
  while (or != NIL) {
    struct exp *result = eval(CAR(or), env);
    if (result != FALSE) {
      return result;
    }
    or = CDR(or);
  }
  return FALSE;
}

// doesn't handle nested quasiquote
static struct exp *expand_quasiquote(struct exp *exp) {
  if (exp->type != PAIR) {
    return exp_make_list(exp_make_symbol("quote"),
                         exp,
                         NULL);
  }
  err_ensure(!is_tagged(exp, "unquote-splicing"),
             "expand_quasiquote: unexpected unquote-splicing");
  if (is_tagged(exp, "unquote")) {
    err_ensure(exp_list_length(exp) == 2,
               "expand_quasiquote: bad syntax in unquote");
    return exp_nth(exp, 1);
  } else if (is_tagged(CAR(exp), "unquote-splicing")) {
    err_ensure(exp_list_length(CAR(exp)) == 2,
               "expand_quasiquote: bad syntax in unquote-splicing");
    return exp_make_list(exp_make_symbol("append"),
                         exp_nth(CAR(exp), 1),
                         expand_quasiquote(CDR(exp)),
                         NULL);
  } else {
      return exp_make_list(exp_make_symbol("cons"),
                           expand_quasiquote(CAR(exp)),
                           expand_quasiquote(CDR(exp)),
                           NULL);
  }
}

static struct exp *expand_begin(struct exp *forms) {
  struct exp *lambda;
  lambda = exp_make_list(exp_make_symbol("lambda"),
                         NIL,
                         NULL);
  CDR(lambda)->value.pair.rest = exp_copy(forms);
  return exp_make_list(lambda, NULL);
}

static struct exp *expand_cond(struct exp *conds) {
  if (conds == NIL) {
    return OK;
  } else {
    struct exp *pred = CAR(CAR(conds));
    return exp_make_list(exp_make_atom("if"),
                         exp_symbol_eq(pred, "else") ? TRUE : pred,
                         exp_nth(CAR(conds), 1),
                         expand_cond(CDR(conds)),
                         NULL);
  }
}

static int is_self_eval(struct exp *exp) {
#define TYPEOF(t) (exp->type == (t))
  return TYPEOF(FIXNUM) || TYPEOF(BOOLEAN) || TYPEOF(STRING) ||
    TYPEOF(VECTOR) || TYPEOF(CLOSURE) || TYPEOF(FUNCTION);
#undef TYPEOF
}

static int is_var(struct exp *exp) {
  return exp->type == SYMBOL;
}

static int is_tagged(struct exp *exp, const char *s) {
  return exp->type == PAIR && exp_symbol_eq(exp->value.pair.first, s);
}

static int is_apply(struct exp *exp) {
  return exp->type == PAIR;
}

static struct env *extend_env(struct exp *params, struct exp *args,
                              struct env *parent) {
  struct env *env = gc_alloc_env(parent);
  for (;;) {
    if (params == NIL && args == NIL) {
      return env;
    } else if (params == NIL) {
      return err_error("apply: too many args");
    } else if (params->type == PAIR) {
      if (args == NIL) {
        return err_error("apply: too few args");
      }
      // ignore non-symbol params
      if (CAR(params)->type == SYMBOL) {
        env_define(env, CAR(params), CAR(args));
      }
      params = CDR(params);
      args = CDR(args);
    } else if (params->type == SYMBOL) {
      env_define(env, params, args);
      return env;
    }
  }
}
