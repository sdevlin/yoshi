#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "exp.h"
#include "err.h"
#include "strbuf.h"

static void eat_space(FILE *infile);
static void eat_until(FILE *infile, int c);
static struct exp *read_atom(FILE *infile);
static struct exp *read_pair(FILE *infile);
static struct exp *read_string(FILE *infile);
static struct exp *read_hash(FILE *infile);

struct exp *read(FILE *infile) {
  int c;
  eat_space(infile);
  c = getc(infile);
  switch (c) {
  case EOF:
    return NULL;
  case '(':
    return read_pair(infile);
  case ')':
    return err_error("extra close parenthesis");
  case '"':
    return read_string(infile);
  case '#':
    return read_hash(infile);
  case '\'':
    return exp_make_list(exp_make_atom("quote"), read(infile), NULL);
  case '`':
    return exp_make_list(exp_make_atom("quasiquote"), read(infile), NULL);
  case ',':
    if ((c = getc(infile)) == '@') {
      return exp_make_list(exp_make_atom("unquote-splicing"),
                           read(infile), NULL);
    } else {
      ungetc(c, infile);
      return exp_make_list(exp_make_atom("unquote"), read(infile), NULL);
    }
  case ';':
    eat_until(infile, '\n');
    return read(infile);
  default:
    ungetc(c, infile);
    return read_atom(infile);
  }
}

static void eat_space(FILE *infile) {
  for (;;) {
    int c = getc(infile);
    if (!isspace(c)) {
      ungetc(c, infile);
      return;
    }
  }
}

static void eat_until(FILE *infile, int c) {
  for (;;) {
    if (c == getc(infile)) {
      return;
    }
  }
}

static struct exp *read_atom(FILE *infile) {
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

static struct exp *read_pair(FILE *infile) {
  int c;
  eat_space(infile);
  if ((c = getc(infile)) == ')') {
    return NIL;
  } else {
    ungetc(c, infile);
    struct exp *exp = read(infile);
    if (exp_symbol_eq(exp, ".")) {
      exp = read(infile);
      eat_space(infile);
      if ((c = getc(infile)) != ')') {
        ungetc(c, infile);
        return err_error("bad dot syntax");
      }
      return exp;
    } else {
      return exp_make_pair(exp, read_pair(infile));
    }
  }
}

static struct exp *read_string(FILE *infile) {
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

static struct exp *read_vector(FILE *infile) {
  int c;
  while ((c = getc(infile)) != ')') {

  }
  return NIL;
}

static struct exp *read_char(FILE *infile) {
  return NIL;
}

static struct exp *read_hash(FILE *infile) {
  int c = getc(infile);
  switch (c) {
  case '(':
    return read_vector(infile);
  case '\\':
    return read_char(infile);
  case 't':
    return TRUE;
  case 'f':
    return FALSE;
  default:
    return err_error("bad syntax in #");
  }
}
