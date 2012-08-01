/*
 * This is a LISP interpreter written in C.
 * It leaks memory like a sieve. Deal with it.
 * RIP John McCarthy and Dennis Ritchie.
 */

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct exp;
struct env;

static struct env global_env;

static struct exp *read(void);
static struct exp *eval(struct exp *exp, struct env *env);
static void print(struct exp *exp);

int main(int argc, char **argv) {
  struct exp *e;
  for (;;) {
    printf("lisp> ");
    if ((e = read()) == NULL) {
      return 0;
    }
    print(eval(e, &global_env));
  }
}

enum exp_type {
  PAIR = 1,
  FIXNUM,
  SYMBOL,
  BOOLEAN,
  SPECIAL
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
  } value;
};

static struct exp nil;
#define NIL (&nil)

static struct exp ok;
#define OK (&ok)

static struct exp true = { BOOLEAN };
#define TRUE (&true)

static struct exp false = { BOOLEAN };
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
static int is_tagged(struct exp *exp, const char *s);
static int is_app(struct exp *exp);
static struct exp *car(struct exp *exp);
static struct exp *cdr(struct exp *exp);
static struct exp *nth(struct exp *exp, size_t n);
static void define_var(struct env *env, char *symbol, struct exp *value);
static struct exp *lookup_var(struct env *env, char *symbol);
static void update_var(struct env *env, char *symbol, struct exp *value);

static struct exp *eval(struct exp *exp, struct env *env) {
  if (is_self_eval(exp)) {
    return exp;
  } else if (is_var(exp)) {
    return lookup_var(env, exp->value.symbol);
  } else if (is_tagged(exp, "quote")) {
    return nth(exp, 1);
  } else if (is_tagged(exp, "set!")) {
    update_var(env, nth(exp, 1)->value.symbol, eval(nth(exp, 2), env));
    return OK;
  } else if (is_tagged(exp, "define")) {
    define_var(env, nth(exp, 1)->value.symbol, eval(nth(exp, 2), env));
    return OK;
  } else if (is_tagged(exp, "if")) {
    if (eval(nth(exp, 1), env) != FALSE) {
      return eval(nth(exp, 2), env);
    } else {
      return eval(nth(exp, 3), env);
    }
  } else if (is_tagged(exp, "lambda")) {
    return NIL; // make_proc
  } else if (is_tagged(exp, "begin")) {
    return NIL; // eval all and return last
  } else if (is_tagged(exp, "cond")) {
    return NIL; // eval(cond_to_if(exp), env);
  } else if (is_app(exp)) {
    return NIL; // apply
  } else {
    fprintf(stderr, "unknown exp type\n");
    exit(1);
  }
}

static int is_self_eval(struct exp *exp) {
  return exp->type == FIXNUM || exp->type == BOOLEAN;
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

static int is_app(struct exp *exp) {
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

static void define_var(struct env *env, char *symbol, struct exp *value) {
  struct binding *b = malloc(sizeof *b);
  b->symbol = symbol;
  b->value = value;
  b->next = env->bindings;
  env->bindings = b;
}

static struct exp *lookup_var(struct env *env, char *symbol) {
  do {
    struct binding *b = env->bindings;
    while (b != NULL) {
      if (!strcmp(b->symbol, symbol)) {
        return b->value;
      }
      b = b->next;
    }
    env = env->parent;
  } while (env);
  fprintf(stderr, "lookup_var: no binding for %s\n", symbol);
  exit(1);
}

static void update_var(struct env *env, char *symbol, struct exp *value) {
  do {
    struct binding *b = env->bindings;
    while (b != NULL) {
      if (!strcmp(b->symbol, symbol)) {
        b->value = value;
        return;
      }
      b = b->next;
    }
    env = env->parent;
  } while (env);
  fprintf(stderr, "update_var: no binding for %s\n", symbol);
  exit(1);
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
  case BOOLEAN:
    buf = malloc(3);
    if (exp == TRUE) {
      strcpy(buf, "#t");
    } else if (exp == FALSE) {
      strcpy(buf, "#f");
    } else {
      fprintf(stderr, "stringify: bad boolean\n");
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
  default:
    fprintf(stderr, "stringify: bad exp type: %d\n", exp->type);
    exit(1);
  }
}

#undef CAT

static void print(struct exp *exp) {
  if (exp == OK) {
    return;
  } else {
    char *str = stringify(exp);
    printf("%s\n", str);
    free(str);
  }
}
