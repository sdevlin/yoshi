#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct exp;
struct env;

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
    print(eval(e, NULL));
  }
}

enum exp_type {
  PAIR = 1,
  FIXNUM,
  SYMBOL,
  BOOLEAN
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
static void define_var(struct env *env, char *symbol, struct exp *exp);
static struct exp *lookup_var(struct env *env, char *symbol);
static void update_var(struct env *env, char *symbol, struct exp *exp);

static struct exp *eval(struct exp *exp, struct env *env) {
  if (is_self_eval(exp)) {
    return exp;
  } else if (is_var(exp)) {
    return lookup_var(env, exp->value.symbol);
  } else if (is_tagged(exp, "quote")) {
    return NIL; // quote_text(exp);
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

static void define_var(struct env *env, char *symbol, struct exp *exp) {

}

static struct exp *lookup_var(struct env *env, char *symbol) {
  return NIL;
}

static void update_var(struct env *env, char *symbol, struct exp *exp) {

}

static void print(struct exp *exp) {
  switch (exp->type) {
  case SYMBOL:
    printf("%s\n", exp->value.symbol);
    break;
  case FIXNUM:
    printf("%ld\n", exp->value.fixnum);
    break;
  case PAIR:
    while (exp != NIL) {
      print(exp->value.pair.first);
      exp = exp->value.pair.rest;
    }
    break;
  case BOOLEAN:
    if (exp == TRUE) {
      printf("#t\n");
    } else if (exp == FALSE) {
      printf("#f\n");
    } else {
      fprintf(stderr, "unexpected boolean\n");
    }
    break;
  default:
    fprintf(stderr, "unexpected exp type: %d\n", exp->type);
  }
}
