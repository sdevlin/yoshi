#include <stdlib.h>

#include "exp.h"
#include "env.h"
#include "err.h"
#include "gc.h"
#include "interp.h"

static int is_self_eval(struct exp *exp);
static int is_var(struct exp *exp);
static int is_tagged(struct exp *exp, const char *s);
static int is_apply(struct exp *exp);
static struct exp *eval_quasiquote(struct exp *exp, struct env *env);
static struct exp *eval_if(struct exp *if_, struct env *env);
static struct exp *eval_and(struct exp *and, struct env *env);
static struct exp *eval_or(struct exp *or, struct env *env);
static struct exp *eval_begin(struct exp *begin, struct env *env);
static struct exp *cond_to_if(struct exp *cond);
static struct env *extend_env(struct exp *params, struct exp *args,
                              struct env *parent);
static struct exp *map_list(struct exp *list, struct env *env,
                            struct exp *(*fn)(struct exp *exp,
                                              struct env *env));

struct exp *eval(struct exp *exp, struct env *env) {
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
    return eval_if(CDR(exp), env);
  } else if (is_tagged(exp, "and")) {
    return eval_and(CDR(exp), env);
  } else if (is_tagged(exp, "or")) {
    return eval_or(CDR(exp), env);
  } else if (is_tagged(exp, "lambda")) {
    return exp_make_closure(exp_nth(exp, 1), CDR(CDR(exp)), env);
  } else if (is_tagged(exp, "begin")) {
    return eval_begin(CDR(exp), env);
  } else if (is_tagged(exp, "cond")) {
    return eval(cond_to_if(CDR(exp)), env);
  } else if (is_apply(exp)) {
    struct exp *fn = eval(CAR(exp), env);
    struct exp *args = map_list(CDR(exp), env, &eval);
    return apply(fn, args, env);
  } else {
    return err_error("unknown exp type");
  }
}

struct exp *apply(struct exp *fn, struct exp *args, struct env *env) {
  switch (fn->type) {
  case FUNCTION:
    return (*fn->value.function)(args);
  case CLOSURE:
    {
      struct env *new_env = extend_env(fn->value.closure.params, args,
                                       fn->value.closure.env);
      struct exp *exp = fn->value.closure.body;
      struct exp *result = OK;
      while (exp != NIL) {
        result = eval(CAR(exp), new_env);
        exp = CDR(exp);
      }
      return result;
    }
  default:
    return err_error("apply: bad function type");
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

static struct exp *eval_if(struct exp *if_, struct env *env) {
  if (eval(CAR(if_), env) != FALSE) {
    return eval(exp_nth(if_, 1), env);
  } else {
    return eval(exp_nth(if_, 2), env);
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

static struct exp *eval_begin(struct exp *forms, struct env *env) {
  struct exp *result = OK;
  while (forms != NIL) {
    result = eval(CAR(forms), env);
    forms = CDR(forms);
  }
  return result;
}

static struct exp *cond_to_if(struct exp *conds) {
  if (conds == NIL) {
    return OK;
  } else {
    struct exp *pred = CAR(CAR(conds));
    return exp_make_list(exp_make_atom("if"),
                         exp_symbol_eq(pred, "else") ? TRUE : pred,
                         exp_nth(CAR(conds), 1),
                         cond_to_if(CDR(conds)),
                         NULL);
  }
}

static int is_self_eval(struct exp *exp) {
  return exp->type == FIXNUM || exp->type == STRING || exp->type == BOOLEAN;
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
