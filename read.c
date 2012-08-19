#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "data.h"
#include "interp.h"
#include "exp.h"
#include "err.h"
#include "strbuf.h"

extern FILE *infile;

static void eat_space(void);
static void eat_until(int c);
static struct exp *read_atom(void);
static struct exp *read_pair(void);
static struct exp *read_string(void);

struct exp *read(void) {
  int c;
  eat_space();
  c = getc(infile);
  switch (c) {
  case EOF:
    return NULL;
  case '(':
    return read_pair();
  case ')':
    return err_error("extra close parenthesis");
  case '"':
    return read_string();
  case '\'':
    return exp_make_list(exp_make_atom("quote"), read(), NULL);
  case '`':
    return exp_make_list(exp_make_atom("quasiquote"), read(), NULL);
  case ',':
    if ((c = getc(infile)) == '@') {
      return exp_make_list(exp_make_atom("unquote-splicing"), read(), NULL);
    } else {
      ungetc(c, infile);
      return exp_make_list(exp_make_atom("unquote"), read(), NULL);
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
  struct strbuf *buf = strbuf_make(0);
  for (;;) {
    int c = getc(infile);
    if (isspace(c) || c == '(' || c == ')') {
      ungetc(c, infile);
      char *str = strbuf_to_cstr(buf);
      struct exp *e = exp_make_atom(str);
      free(str);
      strbuf_free(buf);
      return e;
    } else {
      strbuf_push(buf, c);
    }
  }
}

static struct exp *read_pair(void) {
  int c;
  eat_space();
  if ((c = getc(infile)) == ')') {
    return NIL;
  } else {
    ungetc(c, infile);
    struct exp *exp = read();
    if (exp_symbol_eq(exp, ".")) {
      exp = read();
      eat_space();
      if ((c = getc(infile)) != ')') {
        ungetc(c, infile);
        return err_error("bad dot syntax");
      }
      return exp;
    } else {
      return exp_make_pair(exp, read_pair());
    }
  }
}

static struct exp *read_string(void) {
  struct strbuf *buf = strbuf_make(0);
  int c;
  while ((c = getc(infile)) != '"') {
    if (c == '\\') {
      c = getc(infile);
      if (c == 'n') {
        strbuf_push(buf, '\n');
      } else {
        strbuf_push(buf, c);
      }
    } else {
      strbuf_push(buf, c);
    }
  }
  struct exp *string = exp_make_string(strbuf_to_cstr(buf));
  strbuf_free(buf);
  return string;
}
