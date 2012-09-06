#include <stdlib.h>

#include "exp.h"
#include "env.h"
#include "err.h"
#include "gc_alloc.h"

static int is_self_eval(struct exp *exp);
static int is_var(struct exp *exp);
static int is_tagged(struct exp *exp, const char *s);
static int is_apply(struct exp *exp);
static struct exp *eval_quasiquote(struct exp *exp, struct env *env);
static struct exp *eval_and(struct exp *and, struct env *env);
static struct exp *eval_or(struct exp *or, struct env *env);
static struct exp *expand_begin(struct exp *begin);
static struct exp *expand_cond(struct exp *cond);
static struct env *extend_env(struct exp *params, struct exp *args,
                              struct env *parent);
static struct exp *map_list(struct exp *list, struct env *env,
                            struct exp *(*fn)(struct exp *exp,
                                              struct env *env));

struct exp *eval(struct exp *exp, struct env *env) {
  for (;;) {
    if (is_self_eval(exp)) {
      return exp;
    } else if (is_var(exp)) {
      return env_lookup(env, exp);
    } else if (is_tagged(exp, "quote")) {
      return exp_nth(exp, 1);
    } else if (is_tagged(exp, "quasiquote")) {
      return eval_quasiquote(exp_nth(exp, 1), env);
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

// the messiest, least coherent function in the whole program
// and probably the buggiest
// there must be a better way, but i don't know what it is
static struct exp *eval_quasiquote(struct exp *exp, struct env *env) {
  if (exp->type == PAIR) {
    struct exp *list = NULL;
    struct exp *node = NULL;
    while (exp->type == PAIR) {
      struct exp *item = CAR(exp);
      int is_splice = 0;
      if (is_tagged(item, "unquote")) {
        item = eval(exp_nth(item, 1), env);
      } else if (is_tagged(item, "unquote-splicing")) {
        item = eval(exp_nth(item, 1), env);
        is_splice = 1;
      }
      if (is_splice) {
        if (node == NULL) {
          list = node = item;
        } else {
          node->value.pair.rest = item;
        }
        while (node->value.pair.rest->type == PAIR) {
          node = node->value.pair.rest;
        }
      } else {
        if (node == NULL) {
          list = node = exp_make_pair(item, NULL);
        } else {
          node->value.pair.rest = exp_make_pair(NULL, NULL);
          node = node->value.pair.rest;
          node->value.pair.first = item;
        }
      }
      exp = CDR(exp);
    }
    if (node->value.pair.rest == NULL) {
      node->value.pair.rest = exp;
    }
    return list;
  } else {
    return exp;
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
