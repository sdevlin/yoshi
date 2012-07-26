#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

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
  ATOM,
  PAIR
};

struct exp {
  enum exp_type type;
  union {
    char *atom;
    struct {
      struct exp *first;
      struct exp *rest;
    } pair;
  } value;
};

static struct exp nil = { PAIR };
#define NIL (&nil)

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

static struct exp *make_atom(char *atom) {
  struct exp *e = malloc(sizeof *e);
  e->type = ATOM;
  e->value.atom = atom;
  return e;
}

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

static struct exp *make_pair(void) {
  struct exp *pair = malloc(sizeof *pair);
  pair->type = PAIR;
  pair->value.pair.first = NULL;
  pair->value.pair.rest = NULL;
  return pair;
}

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

static struct exp *eval(struct exp *exp, struct env *env) {
  return exp;
}

static void print(struct exp *exp) {
  switch (exp->type) {
  case ATOM:
    printf("%s\n", exp->value.atom);
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
