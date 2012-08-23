#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "exp.h"
#include "err.h"
#include "gc_alloc.h"
#include "strbuf.h"

struct exp nil = { NIL_TYPE };
struct exp ok = { UNDEFINED };
struct exp true = { BOOLEAN };
struct exp false = { BOOLEAN };

struct exp *exp_make_atom(char *str) {
  if (!strcmp(str, "#t")) {
    return TRUE;
  } else if (!strcmp(str, "#f")) {
    return FALSE;
  } else {
    size_t len = strlen(str);
    size_t i = str[0] == '-' ? 1 : 0;
    struct exp *e = gc_alloc_exp(SYMBOL);
    for (; i < len; i += 1) {
      if (isdigit(str[i])) {
        e->type = FIXNUM;
      } else {
        e->type = SYMBOL;
        break;
      }
    }
    switch (e->type) {
    case SYMBOL:
      e->value.symbol = malloc(len + 1);
      strcpy(e->value.symbol, str);
      break;
    case FIXNUM:
      // not handling overflow
      e->value.fixnum = strtol(str, NULL, 10);
      break;
    default:
      err_error("unexpected atom type");
      break;
    }
    return e;
  }
}

struct exp *exp_make_list(struct exp *first, ...) {
  struct exp *list;
  struct exp *node;
  va_list args;
  assert(first != NULL);
  list = node = exp_make_pair(first, NULL);
  va_start(args, first);
  for (;;) {
    struct exp *next = va_arg(args, struct exp *);
    if (next == NULL) {
      node->value.pair.rest = NIL;
      break;
    } else {
      node->value.pair.rest = exp_make_pair(next, NULL);
      node = node->value.pair.rest;
    }
  }
  va_end(args);
  return list;
}

struct exp *exp_make_string(char *str) {
  struct exp *string = gc_alloc_exp(STRING);
  string->value.string = str;
  return string;
}

struct exp *exp_make_pair(struct exp *first, struct exp *rest) {
  struct exp *pair = gc_alloc_exp(PAIR);
  pair->value.pair.first = first;
  pair->value.pair.rest = rest;
  return pair;
}

// refactor make_atom to use this
struct exp *exp_make_fixnum(long fixnum) {
  struct exp *e = gc_alloc_exp(FIXNUM);
  e->value.fixnum = fixnum;
  return e;
}

struct exp *exp_make_closure(struct exp *params, struct exp *body,
                             struct env *env) {
  struct exp *e = gc_alloc_exp(CLOSURE);
  e->value.closure.params = params;
  e->value.closure.body = body;
  e->value.closure.env = env;
  return e;
}

int exp_symbol_eq(struct exp *exp, const char *s) {
  return exp->type == SYMBOL && !strcmp(exp->value.symbol, s);
}

size_t exp_list_length(struct exp *list) {
  size_t len = 0;
  while (list != NIL) {
    list = CDR(list);
    len += 1;
  }
  return len;
}

struct exp *exp_nth(struct exp *list, size_t n) {
  return n == 0 ? CAR(list) : exp_nth(CDR(list), n - 1);
}

#define CAT(s)                                  \
  do {                                          \
    len += strlen(s);                           \
    while (len + 1 > cap) {                     \
      cap *= 2;                                 \
    }                                           \
    str = realloc(str, cap);                    \
    strcat(str, s);                             \
    str[len] = '\0';                            \
  } while (0)

char *exp_stringify(struct exp *exp) {
  char *str;
  size_t len;
  switch (exp->type) {
  case SYMBOL:
    str = malloc(strlen(exp->value.symbol) + 1);
    strcpy(str, exp->value.symbol);
    return str;
  case STRING:
    {
      struct strbuf *buf = strbuf_make(0);
      size_t i;
      len = strlen(exp->value.string);
      strbuf_push(buf, '"');
      for (i = 0; i < len; i += 1) {
        char c = exp->value.string[i];
        switch (c) {
        case '\n':
          strbuf_push(buf, '\\');
          strbuf_push(buf, 'n');
          break;
        case '"':
          strbuf_push(buf, '\\');
          strbuf_push(buf, '"');
          break;
        default:
          strbuf_push(buf, c);
          break;
        }
      }
      strbuf_push(buf, '"');
      str = strbuf_to_cstr(buf);
      strbuf_free(buf);
      return str;
    }
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
    str = malloc(len + 1);
    sprintf(str, "%ld", exp->value.fixnum);
    return str;
  case BOOLEAN:
    str = malloc(3);
    if (exp == TRUE) {
      strcpy(str, "#t");
    } else if (exp == FALSE) {
      strcpy(str, "#f");
    } else {
      err_error("exp_stringify: bad boolean");
    }
    return str;
  case NIL_TYPE:
    str = malloc(3);
    if (exp == NIL) {
      strcpy(str, "()");
    } else {
      err_error("exp_stringify: bad constant");
    }
    return str;
  case PAIR:
    {
      size_t cap = 1;
      str = malloc(cap);
      len = 0;
      str[len] = '\0';
      CAT("(");
      for (;;) {
        char *s = exp_stringify(CAR(exp));
        CAT(s);
        free(s);
        exp = exp->value.pair.rest;
        if (exp->type == PAIR) {
          CAT(" ");
        } else if (exp == NIL) {
          break;
        } else {
          CAT(" . ");
          s = exp_stringify(exp);
          CAT(s);
          free(s);
          break;
        }
      }
      CAT(")");
      return str;
    }
  case VECTOR:
    str = malloc(3);
    sprintf(str, "%s", "#()");
    return str;
  case FUNCTION:
    str = malloc(13);
    sprintf(str, "%s", "#<function>");
    return str;
  case CLOSURE:
    str = malloc(12);
    sprintf(str, "%s", "#<closure>");
    return str;
  case UNDEFINED:
    str = malloc(13);
    sprintf(str, "%s", "#<undefined>");
    return str;
  default:
    printf("exp type %d\n", exp->type);
    return err_error("exp_stringify: bad exp type");
  }
}

#undef CAT
