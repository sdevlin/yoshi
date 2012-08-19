#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "data.h"
#include "exp.h"
#include "err.h"
#include "gc.h"
#include "read.h"
#include "strbuf.h"

struct exp;
struct env;

struct env global_env;
static void define_primitives(struct env *env);

static struct exp *eval(struct exp *exp, struct env *env);
static void print(struct exp *exp);

static struct {
  struct {
    size_t count;
    char **names;
  } files;
  struct {
    enum flag_type interactive;
    enum flag_type debug;
  } flags;
} config;
FILE *infile;
static FILE *next_file(void);

static void parse_args(int argc, char **argv);

int main(int argc, char **argv) {
  parse_args(argc, argv);
  define_primitives(&global_env);
  infile = next_file();
  for (;;) {
    struct exp *e;
    if (infile == stdin) {
      printf("yoshi> ");
    }
    if (!err_init()) {
      if ((e = read()) == NULL) {
        if ((infile = next_file()) == NULL) {
          return 0;
        } else {
          continue;
        }
      }
      print(eval(e, &global_env));
    } else {
      printf("error: %s\n", err_msg);
    }
    gc_collect();
  }
}

static void parse_args(int argc, char **argv) {
  argc -= 1;
  argv += 1;
  config.files.names = malloc(argc * (sizeof *config.files.names));
  while (argc > 0) {
    char *arg = *argv;
    if (!strcmp(arg, "-i")) {
      config.flags.interactive = ON;
    } else if (!strcmp(arg, "-d")) {
      config.flags.debug = ON;
    } else {
      config.files.count += 1;
      config.files.names[config.files.count - 1] = arg;
    }
    argc -= 1;
    argv += 1;
  }
  if (config.files.count == 0) {
    config.flags.interactive = ON;
  }
}

static FILE *next_file(void) {
  static size_t i = 0;
  if (infile == NULL) {
    return fopen("lib/libyoshi.scm", "r");
  } else if (i < config.files.count) {
    FILE *f = fopen(*(config.files.names + i), "r");
    i += 1;
    return f;
  } else if (infile != stdin && config.flags.interactive) {
    return stdin;
  } else {
    return NULL;
  }
}

static int is_self_eval(struct exp *exp);
static int is_var(struct exp *exp);
static int is_tagged(struct exp *exp, const char *s);
static int is_apply(struct exp *exp);
static struct exp *env_define(struct env *env, struct exp *symbol,
                              struct exp *value);
static struct exp *env_lookup(struct env *env, struct exp *symbol);
static struct exp *env_update(struct env *env, struct exp *symbol,
                              struct exp *value);
static struct exp *eval_quasiquote(struct exp *exp, struct env *env);
static struct exp *eval_if(struct exp *if_, struct env *env);
static struct exp *eval_and(struct exp *and, struct env *env);
static struct exp *eval_or(struct exp *or, struct env *env);
static struct exp *eval_begin(struct exp *begin, struct env *env);
static struct exp *cond_to_if(struct exp *cond);
static struct exp *apply(struct exp *fn, struct exp *args, struct env *env);
static struct env *extend_env(struct exp *params, struct exp *args,
                              struct env *parent);
static struct exp *map_list(struct exp *list, struct env *env,
                            struct exp *(*fn)(struct exp *exp,
                                              struct env *env));

static struct exp *eval(struct exp *exp, struct env *env) {
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

static struct exp *apply(struct exp *fn, struct exp *args, struct env *env) {
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

size_t list_length(struct exp *list) {
  size_t len = 0;
  while (list != NIL) {
    list = CDR(list);
    len += 1;
  }
  return len;
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
  while (and != NIL) {
    if (eval(CAR(and), env) == FALSE) {
      return FALSE;
    }
    and = CDR(and);
  }
  return TRUE;
}

static struct exp *eval_or(struct exp *or, struct env *env) {
  while (or != NIL) {
    if (eval(CAR(or), env) != FALSE) {
      return TRUE;
    }
    or = CDR(or);
  }
  return FALSE;
}

static struct exp *eval_begin(struct exp *forms, struct env *env) {
  struct exp *result;
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
  return exp->type == FIXNUM || exp->type == STRING || exp->type == CONSTANT;
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

static struct exp *env_lookup(struct env *env, struct exp *symbol) {
  return env_visit(env, symbol, NULL, &lookup_found, &error_not_found);
}

static struct exp *update_found(struct binding *binding, struct exp *value) {
  binding->value = value;
  return OK;
}

static struct exp *env_update(struct env *env, struct exp *symbol,
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

static struct exp *env_define(struct env *env, struct exp *symbol,
                              struct exp *value) {
  return env_visit(env, symbol, value, &update_found, &define_not_found);
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

static void print(struct exp *exp) {
  switch (exp->type) {
  case UNDEFINED:
    break;
  default:
    {
      char *str = exp_stringify(exp);
      printf("%s\n", str);
      free(str);
    }
    break;
  }
}

static struct exp *fn_number_p(struct exp *args) {
  err_ensure(list_length(args) == 1, "number? requires exactly one argument");
  return CAR(args)->type == FIXNUM ? TRUE : FALSE;
}

static struct exp *fn_pair_p(struct exp *args) {
  err_ensure(list_length(args) == 1, "pair? requires exactly one argument");
  return CAR(args)->type == PAIR ? TRUE : FALSE;
}

static struct exp *fn_symbol_p(struct exp *args) {
  err_ensure(list_length(args) == 1, "symbol? requires exactly one argument");
  return CAR(args)->type == SYMBOL ? TRUE : FALSE;
}

static struct exp *fn_string_p(struct exp *args) {
  err_ensure(list_length(args) == 1, "string? requires exactly one argument");
  return CAR(args)->type == STRING ? TRUE : FALSE;
}

static struct exp *fn_add(struct exp *args) {
  long acc = 0;
  err_ensure(list_length(args) > 0, "+ requires at least one argument");
  while (args != NIL) {
    struct exp *e = CAR(args);
    err_ensure(e->type == FIXNUM, "+ requires numeric arguments");
    acc += e->value.fixnum;
    args = CDR(args);
  }
  return exp_make_fixnum(acc);
}

static struct exp *fn_sub(struct exp *args) {
  size_t len = list_length(args);
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
  err_ensure(list_length(args) > 0, "* requires at least one argument");
  while (args != NIL) {
    struct exp *e = CAR(args);
    err_ensure(e->type == FIXNUM, "* requires numeric arguments");
    acc *= e->value.fixnum;
    args = CDR(args);
  }
  return exp_make_fixnum(acc);
}

static struct exp *fn_div(struct exp *args) {
  err_ensure(list_length(args) == 2, "div requires exactly two arguments");
  struct exp *a = CAR(args);
  struct exp *b = exp_nth(args, 1);
  err_ensure(a->type == FIXNUM && b->type == FIXNUM,
             "div requires numeric arguments");
  return exp_make_fixnum(a->value.fixnum / b->value.fixnum);
}

static struct exp *fn_mod(struct exp *args) {
  err_ensure(list_length(args) == 2, "mod requires exactly two arguments");
  struct exp *a = CAR(args);
  struct exp *b = exp_nth(args, 1);
  err_ensure(a->type == FIXNUM && b->type == FIXNUM,
             "mod requires numeric arguments");
  return exp_make_fixnum(a->value.fixnum % b->value.fixnum);
}

static struct exp *fn_gt(struct exp *args) {
  err_ensure(list_length(args) == 2, "> requires exactly two arguments");
  struct exp *a = exp_nth(args, 0);
  struct exp *b = exp_nth(args, 1);
  err_ensure(a->type == FIXNUM && b->type == FIXNUM,
             "> requires numeric arguments");
  return a->value.fixnum > b->value.fixnum ? TRUE : FALSE;
}

static struct exp *fn_eq(struct exp *args) {
  size_t len = list_length(args);
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
  if (list_length(args) < 2) {
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
  err_ensure(list_length(args) == 2, "cons requires exactly two arguments");
  struct exp *first = exp_nth(args, 0);
  struct exp *rest = exp_nth(args, 1);
  return exp_make_pair(first, rest);
}

static struct exp *fn_car(struct exp *args) {
  err_ensure(list_length(args) == 1, "car requires exactly one argument");
  args = exp_nth(args, 0);
  err_ensure(args->type == PAIR, "car requires a pair argument");
  return CAR(args);
}

static struct exp *fn_cdr(struct exp *args) {
  err_ensure(list_length(args) == 1, "cdr requires exactly one argument");
  args = exp_nth(args, 0);
  err_ensure(args->type == PAIR, "cdr requires a pair argument");
  return CDR(args);
}

static struct exp *fn_eval(struct exp *args) {
  err_ensure(list_length(args) == 1, "eval requires exactly one argument");
  return eval(CAR(args), &global_env);
}

static struct exp *fn_apply(struct exp *args) {
  err_ensure(list_length(args) == 2, "apply requires exactly two arguments");
  return apply(exp_nth(args, 0), exp_nth(args, 1), &global_env);
}

static struct exp *fn_void(struct exp *args) {
  err_ensure(list_length(args) == 0, "void requires exactly zero arguments");
  return OK;
}

static void define_primitive(struct env *env, char *symbol,
                             struct exp *(*function)(struct exp *args)) {
  struct exp *e = gc_alloc_exp(FUNCTION);
  e->value.function = function;
  env_define(env, exp_make_atom(symbol), e);
}

static void define_primitives(struct env *env) {
#define DEFUN(sym, fn) define_primitive(env, sym, &fn)
  DEFUN("number?", fn_number_p);
  DEFUN("pair?", fn_pair_p);
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
  DEFUN("eval", fn_eval);
  DEFUN("apply", fn_apply);
  DEFUN("void", fn_void);
#undef DEFUN
}
