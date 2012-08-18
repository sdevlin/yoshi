#include <assert.h>
#include <ctype.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct exp;
struct env;

static struct env global_env;
static void define_primitives(struct env *env);

static struct exp *read(void);
static struct exp *eval(struct exp *exp, struct env *env);
static void print(struct exp *exp);

static void gc_collect(void);

enum flag_type {
  OFF,
  ON
};

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
static FILE *infile;
static FILE *next_file(void);

static jmp_buf err_env;
static const char *err_msg;

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
    if (!setjmp(err_env)) {
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

static void *ensure(int exp, const char *msg) {
  if (!exp) {
    err_msg = msg;
    longjmp(err_env, 1);
  }
  return NULL;
}

static void *error(const char *msg) {
  return ensure(0, msg);
}

enum exp_type {
  UNDEFINED,
  PAIR,
  FIXNUM,
  SYMBOL,
  CLOSURE,
  FUNCTION,
  CONSTANT
};

enum mark_type {
  FREE,
  KEEP
};

struct exp {
  enum exp_type type;
  union {
    struct {
      struct exp *first;
      struct exp *rest;
    } pair;
    long fixnum;
    char *symbol;
    struct exp *(*function)(struct exp *args);
    struct {
      struct exp *params;
      struct exp *body;
      struct env *env;
    } closure;
  } value;
  enum mark_type mark;
  struct exp *next;
};

static struct exp nil = { CONSTANT };
#define NIL (&nil)

static struct exp ok = { UNDEFINED };
#define OK (&ok)

static struct exp true = { CONSTANT };
#define TRUE (&true)

static struct exp false = { CONSTANT };
#define FALSE (&false)

static void eat_space(void);
static void eat_until(int c);
static struct exp *read_atom(void);
static struct exp *read_pair(void);
static struct exp *make_atom(char *buf);
static struct exp *make_list(struct exp *first, ...);

static struct exp *read(void) {
  int c;
  eat_space();
  c = getc(infile);
  switch (c) {
  case EOF:
    return NULL;
  case '(':
    return read_pair();
  case ')':
    return error("extra close parenthesis");
  case '\'':
    return make_list(make_atom("quote"), read(), NULL);
  case '`':
    return make_list(make_atom("quasiquote"), read(), NULL);
  case ',':
    if ((c = getc(infile)) == '@') {
      return make_list(make_atom("unquote-splicing"), read(), NULL);
    } else {
      ungetc(c, infile);
      return make_list(make_atom("unquote"), read(), NULL);
    }
  case ';':
    eat_until('\n');
    return read();
  default:
    ungetc(c, infile);
    return read_atom();
  }
}

static void eat_space(void) {
  for (;;) {
    int c = getc(infile);
    if (!isspace(c)) {
      ungetc(c, infile);
      return;
    }
  }
}

static void eat_until(int c) {
  for (;;) {
    if (c == getc(infile)) {
      return;
    }
  }
}

static struct exp *read_atom(void) {
  size_t len = 0;
  size_t cap = 8;
  char *buf = malloc(cap);
  for (;;) {
    int c = getc(infile);
    if (isspace(c) || c == '(' || c == ')') {
      ungetc(c, infile);
      buf[len] = '\0';
      struct exp *e = make_atom(buf);
      free(buf);
      return e;
    } else {
      if (len + 1 == cap) {
        cap *= 2;
        buf = realloc(buf, cap);
      }
      buf[len] = c;
      len += 1;
    }
  }
}

static int symbol_eq(struct exp *exp, const char *s);
static struct exp *make_pair(void);

static struct exp *read_pair(void) {
  int c;
  eat_space();
  if ((c = getc(infile)) == ')') {
    return NIL;
  } else {
    ungetc(c, infile);
    struct exp *exp = read();
    if (symbol_eq(exp, ".")) {
      exp = read();
      eat_space();
      if ((c = getc(infile)) != ')') {
        ungetc(c, infile);
        return error("bad dot syntax");
      }
      return exp;
    } else {
      struct exp *pair = make_pair();
      pair->value.pair.first = exp;
      pair->value.pair.rest = read_pair();
      return pair;
    }
  }
}

static struct exp *gc_alloc_exp(enum exp_type type);

// refactor make_atom to use this
static struct exp *make_fixnum(long fixnum) {
  struct exp *e = gc_alloc_exp(FIXNUM);
  e->value.fixnum = fixnum;
  return e;
}

static struct exp *make_list(struct exp *first, ...) {
  struct exp *list;
  struct exp *node;
  va_list args;
  assert(first != NULL);
  list = node = make_pair();
  node->value.pair.first = first;
  va_start(args, first);
  for (;;) {
    struct exp *next = va_arg(args, struct exp *);
    if (next == NULL) {
      node->value.pair.rest = NIL;
      break;
    } else {
      node->value.pair.rest = make_pair();
      node = node->value.pair.rest;
      node->value.pair.first = next;
    }
  }
  va_end(args);
  return list;
}

static struct exp *make_atom(char *buf) {
  if (!strcmp(buf, "#t")) {
    return TRUE;
  } else if (!strcmp(buf, "#f")) {
    return FALSE;
  } else {
    size_t len = strlen(buf);
    size_t i = buf[0] == '-' ? 1 : 0;
    struct exp *e = gc_alloc_exp(SYMBOL);
    for (; i < len; i += 1) {
      if (isdigit(buf[i])) {
        e->type = FIXNUM;
      } else {
        e->type = SYMBOL;
        break;
      }
    }
    switch (e->type) {
    case SYMBOL:
      e->value.symbol = malloc(len + 1);
      strcpy(e->value.symbol, buf);
      break;
    case FIXNUM:
      // not handling overflow
      e->value.fixnum = strtol(buf, NULL, 10);
      break;
    default:
      error("unexpected atom type");
      break;
    }
    return e;
  }
}

static struct exp *make_pair(void) {
  struct exp *pair = gc_alloc_exp(PAIR);
  pair->value.pair.first = NULL;
  pair->value.pair.rest = NULL;
  return pair;
}

static int is_self_eval(struct exp *exp);
static int is_var(struct exp *exp);
static int is_tagged(struct exp *exp, const char *s);
static int is_apply(struct exp *exp);
static struct exp *car(struct exp *exp);
static struct exp *cdr(struct exp *exp);
static struct exp *nth(struct exp *exp, size_t n);
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
static struct exp *make_closure(struct exp *params, struct exp *body,
                                struct env *env);
static struct exp *apply(struct exp *fn, struct exp *args, struct env *env);
static struct env *extend_env(struct exp *params, struct exp *args,
                              struct env *parent);
static struct exp *map_list(struct exp *list, struct env *env,
                            struct exp *(*fn)(struct exp *exp,
                                              struct env *env));

static char *stringify(struct exp *exp);
static struct exp *eval(struct exp *exp, struct env *env) {
  if (is_self_eval(exp)) {
    return exp;
  } else if (is_var(exp)) {
    return env_lookup(env, exp);
  } else if (is_tagged(exp, "quote")) {
    return nth(exp, 1);
  } else if (is_tagged(exp, "quasiquote")) {
    return eval_quasiquote(nth(exp, 1), env);
  } else if (is_tagged(exp, "set!")) {
    return env_update(env, nth(exp, 1), eval(nth(exp, 2), env));
  } else if (is_tagged(exp, "define")) {
    struct exp *id = car(cdr(exp));
    switch (id->type) {
    case SYMBOL:
      return env_define(env, nth(exp, 1), eval(nth(exp, 2), env));
    case PAIR:
      return env_define(env, car(id), make_closure(cdr(id),
                                                   cdr(cdr(exp)),
                                                   env));
    default:
      return error("define: bad syntax");
    }
  } else if (is_tagged(exp, "if")) {
    return eval_if(cdr(exp), env);
  } else if (is_tagged(exp, "and")) {
    return eval_and(cdr(exp), env);
  } else if (is_tagged(exp, "or")) {
    return eval_or(cdr(exp), env);
  } else if (is_tagged(exp, "lambda")) {
    return make_closure(nth(exp, 1), cdr(cdr(exp)), env);
  } else if (is_tagged(exp, "begin")) {
    return eval_begin(cdr(exp), env);
  } else if (is_tagged(exp, "cond")) {
    return eval(cond_to_if(cdr(exp)), env);
  } else if (is_apply(exp)) {
    struct exp *fn = eval(car(exp), env);
    struct exp *args = map_list(cdr(exp), env, &eval);
    return apply(fn, args, env);
  } else {
    return error("unknown exp type");
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
        result = eval(car(exp), new_env);
        exp = cdr(exp);
      }
      return result;
    }
  default:
    return error("apply: bad function type");
  }
}

size_t list_length(struct exp *list) {
  size_t len = 0;
  while (list != NIL) {
    list = cdr(list);
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
    struct exp *new_list = make_pair();
    new_list->value.pair.first = (*fn)(car(list), env);
    new_list->value.pair.rest = map_list(cdr(list), env, fn);
    return new_list;
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
      struct exp *item = car(exp);
      int is_splice = 0;
      if (is_tagged(item, "unquote")) {
        item = eval(nth(item, 1), env);
      } else if (is_tagged(item, "unquote-splicing")) {
        item = eval(nth(item, 1), env);
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
          list = node = make_pair();
          node->value.pair.first = item;
        } else {
          node->value.pair.rest = make_pair();
          node = node->value.pair.rest;
          node->value.pair.first = item;
        }
      }
      exp = cdr(exp);
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
  if (eval(car(if_), env) != FALSE) {
    return eval(nth(if_, 1), env);
  } else {
    return eval(nth(if_, 2), env);
  }
}

static struct exp *eval_and(struct exp *and, struct env *env) {
  while (and != NIL) {
    if (eval(car(and), env) == FALSE) {
      return FALSE;
    }
    and = cdr(and);
  }
  return TRUE;
}

static struct exp *eval_or(struct exp *or, struct env *env) {
  while (or != NIL) {
    if (eval(car(or), env) != FALSE) {
      return TRUE;
    }
    or = cdr(or);
  }
  return FALSE;
}

static struct exp *eval_begin(struct exp *forms, struct env *env) {
  struct exp *result;
  while (forms != NIL) {
    result = eval(car(forms), env);
    forms = cdr(forms);
  }
  return result;
}

static struct exp *cond_to_if(struct exp *conds) {
  if (conds == NIL) {
    return OK;
  } else {
    struct exp *pred = car(car(conds));
    return make_list(make_atom("if"),
                     symbol_eq(pred, "else") ? TRUE : pred,
                     nth(car(conds), 1),
                     cond_to_if(cdr(conds)),
                     NULL);
  }
}

static struct exp *make_closure(struct exp *params, struct exp *body,
                                struct env *env) {
  struct exp *e = gc_alloc_exp(CLOSURE);
  e->value.closure.params = params;
  e->value.closure.body = body;
  e->value.closure.env = env;
  return e;
}

static int is_self_eval(struct exp *exp) {
  return exp->type == FIXNUM || exp->type == CONSTANT;
}

static int is_var(struct exp *exp) {
  return exp->type == SYMBOL;
}

static int symbol_eq(struct exp *exp, const char *s) {
  return exp->type == SYMBOL && !strcmp(exp->value.symbol, s);
}

static int is_tagged(struct exp *exp, const char *s) {
  return exp->type == PAIR && symbol_eq(exp->value.pair.first, s);
}

static int is_apply(struct exp *exp) {
  return exp->type == PAIR;
}

static struct exp *car(struct exp *exp) {
  return exp->value.pair.first;
}

static struct exp *cdr(struct exp *exp) {
  return exp->value.pair.rest;
}

static struct exp *nth(struct exp *exp, size_t n) {
  return n == 0 ? car(exp) : nth(cdr(exp), n - 1);
}

struct env {
  struct binding {
    char *symbol;
    struct exp *value;
    struct binding *next;
  } *bindings;
  struct env *parent;
  enum mark_type mark;
  struct env *next;
};

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
    return error("env: expected symbol");
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
  return error("env: no binding for symbol");
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

static struct env *gc_alloc_env(struct env *parent);

static struct env *extend_env(struct exp *params, struct exp *args,
                              struct env *parent) {
  struct env *env = gc_alloc_env(parent);
  for (;;) {
    if (params == NIL && args == NIL) {
      return env;
    } else if (params == NIL) {
      return error("apply: too many args");
    } else if (params->type == PAIR) {
      if (args == NIL) {
        return error("apply: too few args");
      }
      // ignore non-symbol params
      if (car(params)->type == SYMBOL) {
        env_define(env, car(params), car(args));
      }
      params = cdr(params);
      args = cdr(args);
    } else if (params->type == SYMBOL) {
      env_define(env, params, args);
      return env;
    }
  }
}

#define CAT(str)                                \
  do {                                          \
    len += strlen(str);                         \
    while (len + 1 > cap) {                     \
      cap *= 2;                                 \
    }                                           \
    buf = realloc(buf, cap);                    \
    strcat(buf, str);                           \
    buf[len] = '\0';                            \
  } while (0)

static char *stringify(struct exp *exp) {
  char *buf;
  size_t len;
  switch (exp->type) {
  case SYMBOL:
    buf = malloc(strlen(exp->value.symbol) + 1);
    strcpy(buf, exp->value.symbol);
    return buf;
  case FIXNUM:
    len = 0;
    {
      long n = exp->value.fixnum;
      if (n < 0) {
        len += 1;
      }
      while (n != 0) {
        len += 1;
        n /= 10;
      }
    }
    buf = malloc(len + 1);
    sprintf(buf, "%ld", exp->value.fixnum);
    return buf;
  case CONSTANT:
    buf = malloc(3);
    if (exp == TRUE) {
      strcpy(buf, "#t");
    } else if (exp == FALSE) {
      strcpy(buf, "#f");
    } else if (exp == NIL) {
      strcpy(buf, "()");
    } else {
      error("stringify: bad constant");
    }
    return buf;
  case PAIR:
    {
      size_t cap = 1;
      buf = malloc(cap);
      len = 0;
      buf[len] = '\0';
      CAT("(");
      for (;;) {
        char *s = stringify(car(exp));
        CAT(s);
        free(s);
        exp = exp->value.pair.rest;
        if (exp->type == PAIR) {
          CAT(" ");
        } else if (exp == NIL) {
          break;
        } else {
          CAT(" . ");
          s = stringify(exp);
          CAT(s);
          free(s);
          break;
        }
      }
      CAT(")");
      return buf;
    }
  case FUNCTION:
    buf = malloc(13);
    sprintf(buf, "%s", "#<function>");
    return buf;
  case CLOSURE:
    buf = malloc(12);
    sprintf(buf, "%s", "#<closure>");
    return buf;
  case UNDEFINED:
    buf = malloc(13);
    sprintf(buf, "%s", "#<undefined>");
    return buf;
  default:
    printf("exp type %d\n", exp->type);
    return error("stringify: bad exp type");
  }
}

#undef CAT

static void print(struct exp *exp) {
  switch (exp->type) {
  case UNDEFINED:
    break;
  default:
    {
      char *str = stringify(exp);
      printf("%s\n", str);
      free(str);
    }
    break;
  }
}

static struct exp *fn_number_p(struct exp *args) {
  ensure(list_length(args) == 1, "number? requires exactly one argument");
  return car(args)->type == FIXNUM ? TRUE : FALSE;
}

static struct exp *fn_pair_p(struct exp *args) {
  ensure(list_length(args) == 1, "pair? requires exactly one argument");
  return car(args)->type == PAIR ? TRUE : FALSE;
}

static struct exp *fn_symbol_p(struct exp *args) {
  ensure(list_length(args) == 1, "symbol? requires exactly one argument");
  return car(args)->type == SYMBOL ? TRUE : FALSE;
}

static struct exp *fn_add(struct exp *args) {
  long acc = 0;
  ensure(list_length(args) > 0, "+ requires at least one argument");
  while (args != NIL) {
    struct exp *e = car(args);
    ensure(e->type == FIXNUM, "+ requires numeric arguments");
    acc += e->value.fixnum;
    args = cdr(args);
  }
  return make_fixnum(acc);
}

static struct exp *fn_sub(struct exp *args) {
  size_t len = list_length(args);
  ensure(len > 0, "- requires at least one argument");
  ensure(car(args)->type == FIXNUM, "- requires numeric arguments");
  long acc = car(args)->value.fixnum;
  args = cdr(args);
  if (len == 1) {
    acc *= -1;
  } else {
    while (args != NIL) {
      ensure(car(args)->type == FIXNUM, "- requires numeric arguments");
      acc -= car(args)->value.fixnum;
      args = cdr(args);
    }
  }
  return make_fixnum(acc);
}

static struct exp *fn_mul(struct exp *args) {
  long acc = 1;
  ensure(list_length(args) > 0, "* requires at least one argument");
  while (args != NIL) {
    struct exp *e = car(args);
    ensure(e->type == FIXNUM, "* requires numeric arguments");
    acc *= e->value.fixnum;
    args = cdr(args);
  }
  return make_fixnum(acc);
}

static struct exp *fn_div(struct exp *args) {
  ensure(list_length(args) == 2, "div requires exactly two arguments");
  struct exp *a = car(args);
  struct exp *b = nth(args, 1);
  ensure(a->type == FIXNUM && b->type == FIXNUM,
         "div requires numeric arguments");
  return make_fixnum(a->value.fixnum / b->value.fixnum);
}

static struct exp *fn_mod(struct exp *args) {
  ensure(list_length(args) == 2, "mod requires exactly two arguments");
  struct exp *a = car(args);
  struct exp *b = nth(args, 1);
  ensure(a->type == FIXNUM && b->type == FIXNUM,
         "mod requires numeric arguments");
  return make_fixnum(a->value.fixnum % b->value.fixnum);
}

static struct exp *fn_gt(struct exp *args) {
  ensure(list_length(args) == 2, "> requires exactly two arguments");
  struct exp *a = nth(args, 0);
  struct exp *b = nth(args, 1);
  ensure(a->type == FIXNUM && b->type == FIXNUM,
         "> requires numeric arguments");
  return a->value.fixnum > b->value.fixnum ? TRUE : FALSE;
}

static struct exp *fn_eq(struct exp *args) {
  size_t len = list_length(args);
  ensure(len >= 2, "= requires at least two arguments");
  struct exp *first = car(args);
  ensure(first->type == FIXNUM, "= requires numeric arguments");
  args = cdr(args);
  while (args != NIL) {
    ensure(car(args)->type == FIXNUM, "= requires numeric arguments");
    if (first->value.fixnum != car(args)->value.fixnum) {
      return FALSE;
    }
    args = cdr(args);
  }
  return TRUE;
}

static struct exp *fn_eq_p(struct exp *args) {
  if (list_length(args) < 2) {
    return TRUE;
  } else {
    struct exp *a = nth(args, 0);
    struct exp *b = nth(args, 1);
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
    return fn_eq_p(cdr(args));
  }
}

static struct exp *fn_cons(struct exp *args) {
  ensure(list_length(args) == 2, "cons requires exactly two arguments");
  struct exp *a = nth(args, 0);
  struct exp *b = nth(args, 1);
  struct exp *pair = make_pair();
  pair->value.pair.first = a;
  pair->value.pair.rest = b;
  return pair;
}

static struct exp *fn_car(struct exp *args) {
  ensure(list_length(args) == 1, "car requires exactly one argument");
  args = nth(args, 0);
  ensure(args->type == PAIR, "car requires a pair argument");
  return car(args);
}

static struct exp *fn_cdr(struct exp *args) {
  ensure(list_length(args) == 1, "cdr requires exactly one argument");
  args = nth(args, 0);
  ensure(args->type == PAIR, "cdr requires a pair argument");
  return cdr(args);
}

static struct exp *fn_eval(struct exp *args) {
  ensure(list_length(args) == 1, "eval requires exactly one argument");
  return eval(car(args), &global_env);
}

static struct exp *fn_apply(struct exp *args) {
  ensure(list_length(args) == 2, "apply requires exactly two arguments");
  return apply(nth(args, 0), nth(args, 1), &global_env);
}

static struct exp *fn_void(struct exp *args) {
  ensure(list_length(args) == 0, "void requires exactly zero arguments");
  return OK;
}

static void define_primitive(struct env *env, char *symbol,
                             struct exp *(*function)(struct exp *args)) {
  struct exp *e = gc_alloc_exp(FUNCTION);
  e->value.function = function;
  env_define(env, make_atom(symbol), e);
}

static void define_primitives(struct env *env) {
#define DEFUN(sym, fn) define_primitive(env, sym, &fn)
  DEFUN("number?", fn_number_p);
  DEFUN("pair?", fn_pair_p);
  DEFUN("symbol?", fn_symbol_p);
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

static struct {
  struct {
    struct exp root;
    size_t count;
  } exp;
  struct {
    struct env root;
    size_t count;
  } env;
} gc;

static void gc_mark_env(struct env *env);
static void gc_sweep(void);
static void gc_free_exp(struct exp *exp);
static void gc_free_env(struct env *env);

static void gc_collect(void) {
  gc_mark_env(&global_env);
  gc_sweep();
  if (config.flags.debug) {
    printf("gc: %d exps %d envs\n", gc.exp.count, gc.env.count);
  }
}

static void gc_mark_exp(struct exp *exp) {
  if (exp->mark) {
    return;
  }
  exp->mark = KEEP;
  switch (exp->type) {
  case PAIR:
    gc_mark_exp(exp->value.pair.first);
    gc_mark_exp(exp->value.pair.rest);
    break;
  case CLOSURE:
    gc_mark_exp(exp->value.closure.params);
    gc_mark_exp(exp->value.closure.body);
    gc_mark_env(exp->value.closure.env);
    break;
  default:
    break;
  }
}

static void gc_mark_env(struct env *env) {
  if (env != &global_env && env->mark == KEEP) {
    return;
  }
  env->mark = KEEP;
  struct binding *b = env->bindings;
  while (b != NULL) {
    gc_mark_exp(b->value);
    b = b->next;
  }
  if (env->parent != NULL) {
    gc_mark_env(env->parent);
  }
}

static void gc_sweep(void) {
#define SWEEP(type)                              \
  do {                                           \
    struct type *prev = &gc.type.root;           \
    struct type *curr = prev->next;              \
    while (curr != NULL) {                       \
      if (curr->mark) {                          \
        curr->mark = FREE;                       \
        prev = curr;                             \
        curr = curr->next;                       \
      } else {                                   \
        prev->next = curr->next;                 \
        gc_free_ ## type(curr);                  \
        curr = prev->next;                       \
      }                                          \
    }                                            \
  } while (0)
  SWEEP(exp);
  SWEEP(env);
#undef SWEEP
}

#define MANAGE(type)                            \
  do {                                          \
    e->mark = FREE;                             \
    e->next = gc.type.root.next;                \
    gc.type.root.next = e;                      \
    gc.type.count += 1;                         \
  } while (0)

static struct exp *gc_alloc_exp(enum exp_type type) {
  struct exp *e = malloc(sizeof *e);
  e->type = type;
  MANAGE(exp);
  return e;
}

static struct env *gc_alloc_env(struct env *parent) {
  struct env *e = malloc(sizeof *e);
  e->bindings = NULL;
  e->parent = parent;
  MANAGE(env);
  return e;
}

#undef MANAGE

static void gc_free_exp(struct exp *exp) {
  switch (exp->type) {
  case SYMBOL:
    free(exp->value.symbol);
    break;
  default:
    break;
  }
  free(exp);
  gc.exp.count -= 1;
}

static void gc_free_env(struct env *env) {
  struct binding *prev = NULL;
  struct binding *curr = env->bindings;
  while (curr != NULL) {
    free(curr->symbol);
    prev = curr;
    curr = curr->next;
    free(prev);
  }
  free(env);
  gc.env.count -= 1;
}
