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

static void eat_while(FILE *input, int (*pred)(int c));
static void eat_space(FILE *input);
static void eat_until(FILE *input, int c);
static struct exp *read_atom(FILE *input);
static struct exp *read_pair(FILE *input);
static struct exp *read_string(FILE *input);
static struct exp *read_hash(FILE *input);

struct exp *read(FILE *input) {
  int c;
  eat_space(input);
  c = get(input);
  switch (c) {
  case EOF:
    return NULL;
  case '(':
    return read_pair(input);
  case ')':
    return err_error("extra close parenthesis");
  case '"':
    return read_string(input);
  case '#':
    return read_hash(input);
  case '\'':
    return exp_make_list(exp_make_atom("quote"), read(input), NULL);
  case '`':
    return exp_make_list(exp_make_atom("quasiquote"), read(input), NULL);
  case ',':
    if ((c = get(input)) == '@') {
      return exp_make_list(exp_make_atom("unquote-splicing"),
                           read(input), NULL);
    } else {
      unget(c, input);
      return exp_make_list(exp_make_atom("unquote"), read(input), NULL);
    }
  case ';':
    eat_until(input, '\n');
    return read(input);
  default:
    unget(c, input);
    return read_atom(input);
  }
}

static void eat_while(FILE *input, int (*pred)(int c)) {
  for (;;) {
    int c = get(input);
    if (!(*pred)(c)) {
      unget(c, input);
      return;
    }
  }
}

static void eat_space(FILE *input) {
  eat_while(input, &isspace);
}

static void eat_until(FILE *input, int c) {
  for (;;) {
    if (c == get(input)) {
      return;
    }
  }
}

static struct exp *read_atom(FILE *input) {
  struct strbuf *buf = strbuf_new(0);
  for (;;) {
    int c = get(input);
    if (isspace(c) || c == '(' || c == ')') {
      unget(c, input);
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

static struct exp *read_pair(FILE *input) {
  int c;
  eat_space(input);
  if ((c = get(input)) == ')') {
    return NIL;
  } else {
    unget(c, input);
    struct exp *exp = read(input);
    if (exp_symbol_eq(exp, ".")) {
      exp = read(input);
      eat_space(input);
      if ((c = get(input)) != ')') {
        unget(c, input);
        return err_error("bad dot syntax");
      }
      return exp;
    } else {
      return exp_make_pair(exp, read_pair(input));
    }
  }
}

static struct exp *read_string(FILE *input) {
  struct strbuf *buf = strbuf_new(0);
  int c;
  while ((c = get(input)) != '"') {
    if (c == '\\') {
      c = get(input);
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

static struct exp *read_vector(FILE *input) {
  struct exp *vector = exp_make_vector(0);
  int c;
  eat_space(input);
  while ((c = get(input)) != ')') {
    unget(c, input);
    vector_push(vector->value.vector, read(input));
    eat_space(input);
  }
  return vector;
}

static struct exp *read_char(FILE *input) {
  int c;
  struct strbuf *buf = strbuf_new(0);
  for (;;) {
    c = get(input);
    if (isgraph(c)) {
      strbuf_push(buf, c);
    } else {
      unget(c, input);
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

static struct exp *read_hash(FILE *input) {
  int c = get(input);
  switch (c) {
  case '(':
    return read_vector(input);
  case '\\':
    return read_char(input);
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
