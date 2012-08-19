#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data.h"
#include "exp.h"
#include "err.h"
#include "strbuf.h"

struct exp nil = { CONSTANT };
struct exp ok = { UNDEFINED };
struct exp true = { CONSTANT };
struct exp false = { CONSTANT };

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
  case CONSTANT:
    str = malloc(3);
    if (exp == TRUE) {
      strcpy(str, "#t");
    } else if (exp == FALSE) {
      strcpy(str, "#f");
    } else if (exp == NIL) {
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
