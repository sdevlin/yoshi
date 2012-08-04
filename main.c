#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// TODO: remove all asserts for things that aren't compile-time checks

struct exp;
struct env;

static struct env global_env;
static void define_primitives(struct env *env);

static struct exp *read(void);
static struct exp *eval(struct exp *exp, struct env *env);
static void print(struct exp *exp);

int main(int argc, char **argv) {
  define_primitives(&global_env);
  for (;;) {
    struct exp *e;
    printf("yoshi> ");
    if ((e = read()) == NULL) {
      return 0;
    }
    print(eval(e, &global_env));
  }
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
static struct exp *read_atom(void);
static struct exp *read_pair(void);

static struct exp *read(void) {
  int c;
  eat_space();
  c = getc(stdin);
  switch (c) {
  case EOF:
    return NULL;
  case '(':
    return read_pair();
  case ')':
    fprintf(stderr, "bad input\n");
    exit(1);
  default:
    ungetc(c, stdin);
    return read_atom();
  }
}

static void eat_space(void) {
  for (;;) {
    int c = getc(stdin);
    if (!isspace(c)) {
      ungetc(c, stdin);
      return;
    }
  }
}

static struct exp *make_atom(char *buf);

static struct exp *read_atom(void) {
  size_t len = 0;
  size_t cap = 8;
  char *buf = malloc(cap);
  for (;;) {
    int c = getc(stdin);
    if (isspace(c) || c == '(' || c == ')') {
      ungetc(c, stdin);
      buf[len] = '\0';
      return make_atom(buf);
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

static struct exp *make_pair(void);

static struct exp *read_pair(void) {
  struct exp *pair;
  int c;
  eat_space();
  if ((c = getc(stdin)) == ')') {
    return NIL;
  } else {
    ungetc(c, stdin);
    pair = make_pair();
    pair->value.pair.first = read();
    pair->value.pair.rest = read_pair();
    return pair;
  }
}

// refactor make_atom to use this
static struct exp *make_fixnum(long fixnum) {
  struct exp *e = malloc(sizeof *e);
  e->type = FIXNUM;
  e->value.fixnum = fixnum;
  return e;
}

static struct exp *make_atom(char *buf) {
  if (!strcmp(buf, "#t")) {
    return TRUE;
  } else if (!strcmp(buf, "#f")) {
    return FALSE;
  } else {
    size_t len = strlen(buf);
    size_t i = buf[0] == '-' ? 1 : 0;
    struct exp *e = malloc(sizeof *e);
    e->type = SYMBOL;
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
      e->value.symbol = buf;
      break;
    case FIXNUM:
      // not handling overflow
      e->value.fixnum = strtol(buf, NULL, 10);
      break;
    default:
      fprintf(stderr, "unexpected atom type\n");
      exit(1);
    }
    return e;
  }
}

static struct exp *make_pair(void) {
  struct exp *pair = malloc(sizeof *pair);
  pair->type = PAIR;
  pair->value.pair.first = NULL;
  pair->value.pair.rest = NULL;
  return pair;
}

static int is_self_eval(struct exp *exp);
static int is_var(struct exp *exp);
static int symbol_eq(struct exp *exp, const char *s);
static int is_tagged(struct exp *exp, const char *s);
static int is_apply(struct exp *exp);
static struct exp *car(struct exp *exp);
static struct exp *cdr(struct exp *exp);
static struct exp *nth(struct exp *exp, size_t n);
static struct exp *env_define(struct env *env, char *symbol,
                              struct exp *value);
static struct exp *env_lookup(struct env *env, char *symbol);
static struct exp *env_update(struct env *env, char *symbol,
                              struct exp *value);
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

static struct exp *eval(struct exp *exp, struct env *env) {
  if (is_self_eval(exp)) {
    return exp;
  } else if (is_var(exp)) {
    return env_lookup(env, exp->value.symbol);
  } else if (is_tagged(exp, "quote")) {
    return nth(exp, 1);
  } else if (is_tagged(exp, "set!")) {
    return env_update(env, nth(exp, 1)->value.symbol, eval(nth(exp, 2), env));
  } else if (is_tagged(exp, "define")) {
    return env_define(env, nth(exp, 1)->value.symbol, eval(nth(exp, 2), env));
  } else if (is_tagged(exp, "if")) {
    if (eval(nth(exp, 1), env) != FALSE) {
      return eval(nth(exp, 2), env);
    } else {
      return eval(nth(exp, 3), env);
    }
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
    fprintf(stderr, "unknown exp type\n");
    exit(1);
  }
}

static struct exp *apply(struct exp *fn, struct exp *args, struct env *env) {
  switch (fn->type) {
  case FUNCTION:
    return fn->value.function(args);
  case CLOSURE:
    {
      struct env *new_env = extend_env(fn->value.closure.params, args,
                                       fn->value.closure.env);
      struct exp *exp = fn->value.closure.body;
      struct exp *result;
      while (exp != NIL) {
        result = eval(car(exp), new_env);
        exp = cdr(exp);
      }
      return result;
    }
  default:
    fprintf(stderr, "apply: bad type: %d\n", fn->type);
    exit(1);
  }
}

size_t list_length(struct exp *list) {
  size_t len = 0;
  assert(list->type == PAIR || list == NIL);
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

static struct exp *eval_begin(struct exp *forms, struct env *env) {
  struct exp *result;
  assert(forms != NIL);
  while (forms != NIL) {
    result = eval(car(forms), env);
    forms = cdr(forms);
  }
  return result;
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
    assert(next != NULL);
    if (next == NIL) {
      node->value.pair.rest = next;
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

static struct exp *cond_to_if(struct exp *conds) {
  if (conds == NIL) {
    return OK;
  } else {
    struct exp *pred = car(car(conds));
    return make_list(make_atom("if"),
                     symbol_eq(pred, "else") ? TRUE : pred,
                     nth(car(conds), 1),
                     cond_to_if(cdr(conds)),
                     NIL);
  }
}

static struct exp *make_closure(struct exp *params, struct exp *body,
                                struct env *env) {
  struct exp *e = malloc(sizeof *e);
  e->type = CLOSURE;
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
  assert(exp->type == PAIR);
  return exp->value.pair.first;
}

static struct exp *cdr(struct exp *exp) {
  assert(exp->type == PAIR);
  return exp->value.pair.rest;
}

static struct exp *nth(struct exp *exp, size_t n) {
  assert(exp->type == PAIR);
  return n == 0 ? car(exp) : nth(cdr(exp), n - 1);
}

struct env {
  struct binding {
    char *symbol;
    struct exp *value;
    struct binding *next;
  } *bindings;
  struct env *parent;
};

static struct exp *env_define(struct env *env, char *symbol,
                              struct exp *value) {
  struct binding *b = malloc(sizeof *b);
  b->symbol = symbol;
  b->value = value;
  b->next = env->bindings;
  env->bindings = b;
  return OK;
}

static struct exp *env_visit(struct env *env, char *symbol, struct exp *value,
                             struct exp *(*visit)(struct binding *b,
                                                  struct exp *v)) {
  do {
    struct binding *b = env->bindings;
    while (b != NULL) {
      if (!strcmp(b->symbol, symbol)) {
        return (*visit)(b, value);
      }
      b = b->next;
    }
    env = env->parent;
  } while (env);
  fprintf(stderr, "env_visit: no binding for %s\n", symbol);
  exit(1);
}

static struct exp *binding_lookup(struct binding *binding, struct exp *_) {
  return binding->value;
}

static struct exp *env_lookup(struct env *env, char *symbol) {
  return env_visit(env, symbol, NULL, &binding_lookup);
}

static struct exp *binding_update(struct binding *binding, struct exp *value) {
  binding->value = value;
  return OK;
}

static struct exp *env_update(struct env *env, char *symbol,
                              struct exp *value) {
  return env_visit(env, symbol, value, &binding_update);
}

static struct env *extend_env(struct exp *params, struct exp *args,
                              struct env *parent) {
  struct env *env = malloc(sizeof *env);
  env->parent = parent;
  assert(list_length(params) == list_length(args));
  while (params != NIL) {
    assert(params->type == PAIR);
    assert(car(params)->type == SYMBOL);
    assert(args->type == PAIR);
    env_define(env, car(params)->value.symbol, car(args));
    params = cdr(params);
    args = cdr(args);
  }
  return env;
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
      fprintf(stderr, "stringify: bad constant\n");
      exit(1);
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
        CAT(stringify(exp->value.pair.first));
        exp = exp->value.pair.rest;
        if (exp->type == PAIR) {
          CAT(" ");
        } else if (exp == NIL) {
          break;
        } else {
          CAT(" . ");
          CAT(stringify(exp->value.pair.first));
          break;
        }
      }
      CAT(")");
      return buf;
    }
  case FUNCTION:
    buf = malloc(13);
    sprintf(buf, "%s", "<# function>");
    return buf;
  case CLOSURE:
    buf = malloc(12);
    sprintf(buf, "%s", "<# closure>");
    return buf;
  default:
    fprintf(stderr, "stringify: bad exp type: %d\n", exp->type);
    exit(1);
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

struct exp *fn_add(struct exp *args) {
  long acc = 0;
  assert(list_length(args) > 0);
  while (args != NIL) {
    assert(args->type == PAIR);
    struct exp *e = car(args);
    assert(e->type == FIXNUM);
    acc += e->value.fixnum;
    args = cdr(args);
  }
  return make_fixnum(acc);
}

struct exp *fn_sub(struct exp *args) {
  size_t len = list_length(args);
  assert(len > 0);
  assert(car(args)->type == FIXNUM);
  long acc = car(args)->value.fixnum;
  args = cdr(args);
  if (len == 1) {
    acc *= -1;
  } else {
    while (args != NIL) {
      assert(car(args)->type == FIXNUM);
      acc -= car(args)->value.fixnum;
      args = cdr(args);
    }
  }
  return make_fixnum(acc);
}

static void define_primitive(struct env *env, char *symbol,
                             struct exp *(*function)(struct exp *args)) {
  struct exp *e = malloc(sizeof *e);
  e->type = FUNCTION;
  e->value.function = function;
  env_define(env, symbol, e);
}

static void define_primitives(struct env *env) {
#define DEFUN(sym, fn) define_primitive(env, sym, &fn)
  DEFUN("+", fn_add);
  DEFUN("-", fn_sub);
#undef DEFUN
}
