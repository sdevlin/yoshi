#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "exp.h"
#include "err.h"
#include "util/strbuf.h"
#include "util/vector.h"

static int get(FILE *stream);
static void unget(int c, FILE *stream);

static void eat_while(FILE *infile, int (*pred)(int c));
static void eat_space(FILE *infile);
static void eat_until(FILE *infile, int c);
static struct exp *read_atom(FILE *infile);
static struct exp *read_pair(FILE *infile);
static struct exp *read_string(FILE *infile);
static struct exp *read_hash(FILE *infile);

struct exp *read(FILE *infile) {
  int c;
  eat_space(infile);
  c = get(infile);
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
    if ((c = get(infile)) == '@') {
      return exp_make_list(exp_make_atom("unquote-splicing"),
                           read(infile), NULL);
    } else {
      unget(c, infile);
      return exp_make_list(exp_make_atom("unquote"), read(infile), NULL);
    }
  case ';':
    eat_until(infile, '\n');
    return read(infile);
  default:
    unget(c, infile);
    return read_atom(infile);
  }
}

static void eat_while(FILE *infile, int (*pred)(int c)) {
  for (;;) {
    int c = get(infile);
    if (!(*pred)(c)) {
      unget(c, infile);
      return;
    }
  }
}

static void eat_space(FILE *infile) {
  eat_while(infile, &isspace);
}

static void eat_until(FILE *infile, int c) {
  for (;;) {
    if (c == get(infile)) {
      return;
    }
  }
}

static struct exp *read_atom(FILE *infile) {
  struct strbuf *buf = strbuf_new(0);
  for (;;) {
    int c = get(infile);
    if (isspace(c) || c == '(' || c == ')') {
      unget(c, infile);
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
  if ((c = get(infile)) == ')') {
    return NIL;
  } else {
    unget(c, infile);
    struct exp *exp = read(infile);
    if (exp_symbol_eq(exp, ".")) {
      exp = read(infile);
      eat_space(infile);
      if ((c = get(infile)) != ')') {
        unget(c, infile);
        return err_error("bad dot syntax");
      }
      return exp;
    } else {
      return exp_make_pair(exp, read_pair(infile));
    }
  }
}

static struct exp *read_string(FILE *infile) {
  struct strbuf *buf = strbuf_new(0);
  int c;
  while ((c = get(infile)) != '"') {
    if (c == '\\') {
      c = get(infile);
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
  struct exp *vector = exp_make_vector(0);
  int c;
  eat_space(infile);
  while ((c = get(infile)) != ')') {
    unget(c, infile);
    vector_push(vector->value.vector, read(infile));
    eat_space(infile);
  }
  return vector;
}

static struct exp *read_char(FILE *infile) {
  int c;
  struct strbuf *buf = strbuf_new(0);
  for (;;) {
    c = get(infile);
    if (isgraph(c)) {
      strbuf_push(buf, c);
    } else {
      unget(c, infile);
      break;
    }
  }
  char *name = strbuf_to_cstr(buf);
  strbuf_free(buf);
  if (strlen(name) == 0) {
    return err_error("read_char: zero-length character");
  } else if (strlen(name) == 1) {
    return exp_make_character(*name);
  } else if (*name == 'x') {
    // handle raw hex
    return err_error("read_char: hex characters not implemented");
  } else {
    int c = exp_name_to_char(name);
    if (c == -1) {
      return err_error("read_char: unknown character name");
    }
    return exp_make_character(c);
  }
}

static struct exp *read_hash(FILE *infile) {
  int c = get(infile);
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

static int get(FILE *stream) {
  int c = getc(stream);
  if (isgraph(c) || isspace(c) || c == EOF) {
    return c;
  } else {
    fprintf(stderr, "unexpected char in input: %02x\n", c);
    exit(1);
  }
}

// simple wrapper to ungetc
// provided for symmetry with get
static void unget(int c, FILE *stream) {
  ungetc(c, stream);
}
