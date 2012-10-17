#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "exp.h"
#include "env.h"
#include "err.h"
#include "gc.h"
#include "eval.h"
#include "util/vector.h"

static struct exp *fn_number_p(struct exp *args) {
  err_ensure(exp_list_length(args) == 1,
             "number? requires exactly one argument");
  return CAR(args)->type == FIXNUM ? TRUE : FALSE;
}

static struct exp *fn_pair_p(struct exp *args) {
  err_ensure(exp_list_length(args) == 1,
             "pair? requires exactly one argument");
  return CAR(args)->type == PAIR ? TRUE : FALSE;
}

static struct exp *fn_vector_p(struct exp *args) {
  err_ensure(exp_list_length(args) == 1,
             "vector? requires exactly one argument");
  return CAR(args)->type == VECTOR ? TRUE : FALSE;
}

static struct exp *fn_symbol_p(struct exp *args) {
  err_ensure(exp_list_length(args) == 1,
             "symbol? requires exactly one argument");
  return CAR(args)->type == SYMBOL ? TRUE : FALSE;
}

static struct exp *fn_string_p(struct exp *args) {
  err_ensure(exp_list_length(args) == 1,
             "string? requires exactly one argument");
  return CAR(args)->type == STRING ? TRUE : FALSE;
}

static struct exp *fn_add(struct exp *args) {
  long acc = 0;
  while (args != NIL) {
    struct exp *e = CAR(args);
    err_ensure(e->type == FIXNUM, "+ requires numeric arguments");
    acc += e->value.fixnum;
    args = CDR(args);
  }
  return exp_make_fixnum(acc);
}

static struct exp *fn_sub(struct exp *args) {
  size_t len = exp_list_length(args);
  err_ensure(len > 0, "- requires at least one argument");
  err_ensure(CAR(args)->type == FIXNUM, "- requires numeric arguments");
  long acc = CAR(args)->value.fixnum;
  args = CDR(args);
  if (len == 1) {
    acc *= -1;
  } else {
    while (args != NIL) {
      err_ensure(CAR(args)->type == FIXNUM, "- requires numeric arguments");
      acc -= CAR(args)->value.fixnum;
      args = CDR(args);
    }
  }
  return exp_make_fixnum(acc);
}

static struct exp *fn_mul(struct exp *args) {
  long acc = 1;
  while (args != NIL) {
    struct exp *e = CAR(args);
    err_ensure(e->type == FIXNUM, "* requires numeric arguments");
    acc *= e->value.fixnum;
    args = CDR(args);
  }
  return exp_make_fixnum(acc);
}

static struct exp *fn_div(struct exp *args) {
  err_ensure(exp_list_length(args) == 2, "div requires exactly two arguments");
  struct exp *a = CAR(args);
  struct exp *b = exp_nth(args, 1);
  err_ensure(a->type == FIXNUM && b->type == FIXNUM,
             "div requires numeric arguments");
  return exp_make_fixnum(a->value.fixnum / b->value.fixnum);
}

static struct exp *fn_mod(struct exp *args) {
  err_ensure(exp_list_length(args) == 2, "mod requires exactly two arguments");
  struct exp *a = CAR(args);
  struct exp *b = exp_nth(args, 1);
  err_ensure(a->type == FIXNUM && b->type == FIXNUM,
             "mod requires numeric arguments");
  return exp_make_fixnum(a->value.fixnum % b->value.fixnum);
}

static struct exp *fn_gt(struct exp *args) {
  err_ensure(exp_list_length(args) == 2, "> requires exactly two arguments");
  struct exp *a = exp_nth(args, 0);
  struct exp *b = exp_nth(args, 1);
  err_ensure(a->type == FIXNUM && b->type == FIXNUM,
             "> requires numeric arguments");
  return a->value.fixnum > b->value.fixnum ? TRUE : FALSE;
}

static struct exp *fn_eq(struct exp *args) {
  size_t len = exp_list_length(args);
  err_ensure(len >= 2, "= requires at least two arguments");
  struct exp *first = CAR(args);
  err_ensure(first->type == FIXNUM, "= requires numeric arguments");
  args = CDR(args);
  while (args != NIL) {
    err_ensure(CAR(args)->type == FIXNUM, "= requires numeric arguments");
    if (first->value.fixnum != CAR(args)->value.fixnum) {
      return FALSE;
    }
    args = CDR(args);
  }
  return TRUE;
}

static struct exp *fn_eq_p(struct exp *args) {
  if (exp_list_length(args) < 2) {
    return TRUE;
  } else {
    struct exp *a = exp_nth(args, 0);
    struct exp *b = exp_nth(args, 1);
    if (a->type != b->type) {
      return FALSE;
    } else {
      switch (a->type) {
      case FIXNUM:
        if (a->value.fixnum != b->value.fixnum) {
          return FALSE;
        }
        break;
      case SYMBOL:
        if (strcmp(a->value.symbol, b->value.symbol) != 0) {
          return FALSE;
        }
        break;
      default:
        if (a != b) {
          return FALSE;
        }
        break;
      }
    }
    return fn_eq_p(CDR(args));
  }
}

static struct exp *fn_cons(struct exp *args) {
  err_ensure(exp_list_length(args) == 2,
             "cons requires exactly two arguments");
  struct exp *first = exp_nth(args, 0);
  struct exp *rest = exp_nth(args, 1);
  return exp_make_pair(first, rest);
}

static struct exp *fn_car(struct exp *args) {
  err_ensure(exp_list_length(args) == 1, "car requires exactly one argument");
  args = CAR(args);
  err_ensure(args->type == PAIR, "car requires a pair argument");
  return CAR(args);
}

static struct exp *fn_cdr(struct exp *args) {
  err_ensure(exp_list_length(args) == 1, "cdr requires exactly one argument");
  args = CAR(args);
  err_ensure(args->type == PAIR, "cdr requires a pair argument");
  return CDR(args);
}

static struct exp *fn_make_vector(struct exp *args) {
  size_t len = exp_list_length(args);
  err_ensure(len == 1 || len == 2,
             "make-vector requires exactly one or two arguments");
  return OK;
}

static struct exp *fn_vector_length(struct exp *args) {
  err_ensure(exp_list_length(args) == 1,
             "vector-length requires exactly one argument");
  struct exp *vector = CAR(args);
  err_ensure(vector->type == VECTOR,
             "vector-length requires a vector argument");
  return exp_make_fixnum(vector_length(vector->value.vector));
}

static struct exp *fn_vector_ref(struct exp *args) {
  err_ensure(exp_list_length(args) == 2,
             "vector-ref requires exactly two arguments");
  struct exp *vector = CAR(args);
  struct exp *index = CAR(CDR(args));
  err_ensure(vector->type == VECTOR,
             "vector-ref requires a vector argument");
  err_ensure(index->type == FIXNUM,
             "vector-ref requires a numeric index");
  long i = index->value.fixnum;
  err_ensure(i >= 0 && i < vector_length(vector->value.vector),
             "vector-ref requires a valid index");
  return vector_get(vector->value.vector, i);
}

static struct exp *fn_eval(struct exp *args) {
  err_ensure(exp_list_length(args) == 1,
             "eval requires exactly one argument");
  return eval(CAR(args), &global_env);
}

static struct exp *fn_about(struct exp *args) {
  err_ensure(exp_list_length(args) == 0,
             "about requires exactly zero arguments");
  const char *about = ("I'm not much more than an interpreter, "
                       "and not very good at telling stories.");
  char *buf = malloc(strlen(about) + 1);
  strcpy(buf, about);
  return exp_make_string(buf);
}

static void define_primitive(struct env *env, char *symbol,
                             struct exp *(*function)(struct exp *args)) {
  struct exp *e = (*gc->alloc_exp)(FUNCTION);
  e->value.function = function;
  env_define(env, exp_make_atom(symbol), e);
}

void builtin_define(struct env *env) {
#define DEFUN(sym, fn) define_primitive(env, sym, &fn)
  DEFUN("number?", fn_number_p);
  DEFUN("pair?", fn_pair_p);
  DEFUN("vector?", fn_vector_p);
  DEFUN("symbol?", fn_symbol_p);
  DEFUN("string?", fn_string_p);
  DEFUN("+", fn_add);
  DEFUN("-", fn_sub);
  DEFUN("*", fn_mul);
  DEFUN("div", fn_div);
  DEFUN("mod", fn_mod);
  DEFUN(">", fn_gt);
  DEFUN("=", fn_eq);
  DEFUN("eq?", fn_eq_p);
  DEFUN("cons", fn_cons);
  DEFUN("car", fn_car);
  DEFUN("cdr", fn_cdr);
  DEFUN("make-vector", fn_make_vector);
  DEFUN("vector-length", fn_vector_length);
  DEFUN("vector-ref", fn_vector_ref);
  DEFUN("eval", fn_eval);
  DEFUN("about", fn_about);
#undef DEFUN
}
