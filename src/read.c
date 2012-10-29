#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "exp.h"
#include "err.h"
#include "util/input.h"
#include "util/strbuf.h"
#include "util/vector.h"

static int get(struct input *input);
static void unget(struct input *input, int c);

static void expect(struct input *input, int c);
static void eat_while(struct input *input, int (*pred)(int c));
static void eat_space(struct input *input);
static void eat_until(struct input *input, int c);
static struct exp *read_atom(struct input *input);
static struct exp *read_pair(struct input *input);
static struct exp *read_string(struct input *input);
static struct exp *read_hash(struct input *input);

struct exp *read(struct input *input) {
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
    return exp_quote(read(input));
  case '`':
    return exp_make_list(exp_make_atom("quasiquote"), read(input), NULL);
  case ',':
    if ((c = get(input)) == '@') {
      return exp_make_list(exp_make_atom("unquote-splicing"),
                           read(input), NULL);
    } else {
      unget(input, c);
      return exp_make_list(exp_make_atom("unquote"), read(input), NULL);
    }
  case ';':
    eat_until(input, '\n');
    return read(input);
  default:
    unget(input, c);
    return read_atom(input);
  }
}

static void expect(struct input *input, int c) {
  if (c != get(input)) {
    err_error("read: unexpected character in input");
  }
}

static void eat_while(struct input *input, int (*pred)(int c)) {
  for (;;) {
    int c = get(input);
    if (!(*pred)(c)) {
      unget(input, c);
      return;
    }
  }
}

static void eat_space(struct input *input) {
  eat_while(input, &isspace);
}

static void eat_until(struct input *input, int c) {
  for (;;) {
    if (c == get(input)) {
      return;
    }
  }
}

static struct exp *read_atom(struct input *input) {
  struct strbuf *buf = strbuf_new(0);
  for (;;) {
    int c = get(input);
    if (isspace(c) || c == '(' || c == ')') {
      unget(input, c);
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

static struct exp *read_pair(struct input *input) {
  int c;
  eat_space(input);
  if ((c = get(input)) == ')') {
    return NIL;
  } else {
    unget(input, c);
    struct exp *exp = read(input);
    if (exp_symbol_eq(exp, ".")) {
      exp = read(input);
      eat_space(input);
      if ((c = get(input)) != ')') {
        unget(input, c);
        return err_error("bad dot syntax");
      }
      return exp;
    } else {
      return exp_make_pair(exp, read_pair(input));
    }
  }
}

static struct exp *read_string(struct input *input) {
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

static struct exp *read_vector(struct input *input) {
  struct exp *vector = exp_make_vector(0);
  int c;
  eat_space(input);
  while ((c = get(input)) != ')') {
    unget(input, c);
    vector_push(vector->value.vector, read(input));
    eat_space(input);
  }
  return vector;
}

static struct exp *read_char(struct input *input) {
  int c;
  struct strbuf *buf = strbuf_new(0);
  for (;;) {
    c = get(input);
    if (isgraph(c)) {
      strbuf_push(buf, c);
    } else {
      unget(input, c);
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
    /* handle raw hex */
    return err_error("read_char: hex characters not implemented");
  } else {
    int c = exp_name_to_char(name);
    if (c == -1) {
      return err_error("read_char: unknown character name");
    }
    return exp_make_character(c);
  }
}

static struct exp *read_bytevector(struct input *input) {
  struct exp *bytevector = exp_make_bytevector(0);
  int c;
  eat_space(input);
  while ((c = get(input)) != ')') {
    unget(input, c);
    vector_push(bytevector->value.vector, read(input));
    eat_space(input);
  }
  return bytevector;
}

static struct exp *read_hash(struct input *input) {
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
  case 'u':
    expect(input, '8');
    expect(input, '(');
    return read_bytevector(input);
  default:
    return err_error("bad syntax in #");
  }
}

static int get(struct input *input) {
  int c = input->get(input);
  if (isgraph(c) || isspace(c) || c == EOF) {
    return c;
  } else {
    fprintf(stderr, "unexpected char in input: %02x\n", c);
    exit(1);
  }
}

static void unget(struct input *input, int c) {
  input->unget(input, c);
}
