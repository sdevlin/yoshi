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
  while ((e = read()) != NULL) {
    print(eval(e, NULL));
  }
  return 0;
}

enum exp_type {
  PAIR = 1,
  FIXNUM,
  SYMBOL
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

/*
static struct exp true;
#define TRUE (&true)

static struct exp false;
#define FALSE (&false)
*/

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

static struct exp *eval(struct exp *exp, struct env *env) {
  if (is_self_eval(exp)) {
    return exp;
  } else if (is_var(exp)) {
    return NULL; // lookup_var(exp, env);
  } else if (is_tagged(exp, "quote")) {
    return NULL; // quote_text(exp);
  } else if (is_tagged(exp, "set!")) {
    return NULL;
  } else if (is_tagged(exp, "define")) {
    return NULL;
  } else if (is_tagged(exp, "if")) {
    return NULL;
  } else if (is_tagged(exp, "lambda")) {
    return NULL;
  } else if (is_tagged(exp, "begin")) {
    return NULL;
  } else if (is_tagged(exp, "cond")) {
    return NULL;
  } else if (is_app(exp)) {
    return NULL;
  } else {
    fprintf(stderr, "unknown exp type\n");
    exit(1);
  }
}

static int is_self_eval(struct exp *exp) {
  return exp->type == FIXNUM;
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
  default:
    fprintf(stderr, "unexpected exp type: %d\n", exp->type);
  }
}
