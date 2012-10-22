#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "exp.h"
#include "err.h"
#include "gc.h"
#include "util/strbuf.h"
#include "util/vector.h"

struct exp nil = { .type = NIL_TYPE };
struct exp ok = { .type = UNDEFINED };
struct exp true = { .type = BOOLEAN };
struct exp false = { .type = BOOLEAN };

struct exp *exp_make_atom(const char *str) {
  if (!strcmp(str, "#t")) {
    return TRUE;
  } else if (!strcmp(str, "#f")) {
    return FALSE;
  } else {
    size_t len = strlen(str);
    size_t i = str[0] == '-' ? 1 : 0;
    enum exp_type type = SYMBOL;
    for (; i < len; i += 1) {
      if (isdigit(str[i])) {
        type = FIXNUM;
      } else {
        type = SYMBOL;
        break;
      }
    }
    switch (type) {
    case SYMBOL:
      return exp_make_symbol(str);
    case FIXNUM:
      /* not handling overflow */
      return exp_make_fixnum(strtol(str, NULL, 10));
    default:
      return err_error("unexpected atom type");
    }
  }
}

struct exp *exp_make_symbol(const char *sym) {
  struct exp *symbol = (*gc->alloc_exp)(SYMBOL);
  symbol->value.symbol = malloc(strlen(sym) + 1);
  strcpy(symbol->value.symbol, sym);
  return symbol;
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

struct exp *exp_make_vector(size_t len, ...) {
  va_list args;
  size_t i;
  struct exp *vector = (*gc->alloc_exp)(VECTOR);
  vector->value.vector = vector_new(len);
  va_start(args, len);
  for (i = 0; i < len; i += 1) {
    vector_push(vector->value.vector, va_arg(args, struct exp *));
  }
  va_end(args);
  return vector;
}

struct exp *exp_make_string(char *str) {
  struct exp *string = (*gc->alloc_exp)(STRING);
  string->value.string = str;
  return string;
}

struct exp *exp_make_character(int c) {
  struct exp *character = (*gc->alloc_exp)(CHARACTER);
  character->value.character = c;
  return character;
}

struct exp *exp_make_pair(struct exp *first, struct exp *rest) {
  struct exp *pair = (*gc->alloc_exp)(PAIR);
  pair->value.pair.first = first;
  pair->value.pair.rest = rest;
  return pair;
}

struct exp *exp_make_fixnum(long fixnum) {
  struct exp *e = (*gc->alloc_exp)(FIXNUM);
  e->value.fixnum = fixnum;
  return e;
}

struct exp *exp_make_closure(struct exp *params, struct exp *body,
                             struct env *env) {
  struct exp *e = (*gc->alloc_exp)(CLOSURE);
  e->value.closure.params = params;
  e->value.closure.body = body;
  e->value.closure.env = env;
  return e;
}

struct exp *exp_copy(struct exp *exp) {
  assert(exp != NULL);
  switch (exp->type) {
  case UNDEFINED:
  case BOOLEAN:
  case NIL_TYPE:
    return exp;
  case PAIR:
    return exp_make_pair(exp_copy(exp->value.pair.first),
                         exp_copy(exp->value.pair.rest));
  case FIXNUM:
    return exp_make_fixnum(exp->value.fixnum);
  case SYMBOL:
    return exp_make_symbol(exp->value.symbol);
  case STRING:
    return exp_make_string(exp->value.string);
  default:
    return err_error("unsupported type for copying");
  }
}

int exp_symbol_eq(struct exp *exp, const char *s) {
  return IS(exp, SYMBOL) && !strcmp(exp->value.symbol, s);
}

struct char_name {
  const char *name;
  int value;
};

static struct char_name char_map[] = {
  { .name = "alarm", .value = '\a' },
  { .name = "backspace", .value = '\b' },
  { .name = "delete", .value = 0x7f },
  { .name = "escape", .value = 0x1b },
  { .name = "newline", .value = '\n' },
  { .name = "null", .value = '\0' },
  { .name = "return", .value = '\r' },
  { .name = "space", .value = ' ' },
  { .name = "tab", .value = '\t' },
};

#define NELEM(arr) ((sizeof arr) / (sizeof arr[0]))

int exp_name_to_char(const char *name) {
  size_t i;
  for (i = 0; i < NELEM(char_map); i += 1) {
    if (!strcmp(name, char_map[i].name)) {
      return char_map[i].value;
    }
  }
  return -1;
}

const char *exp_char_to_name(int c) {
  size_t i;
  for (i = 0; i < NELEM(char_map); i += 1) {
    if (c == char_map[i].value) {
      return char_map[i].name;
    }
  }
  return NULL;
}

#undef NELEM

size_t exp_list_length(struct exp *list) {
  size_t len = 0;
  err_ensure(exp_list_proper(list), "argument must be proper list");
  while (list != NIL) {
    list = CDR(list);
    len += 1;
  }
  return len;
}

int exp_list_tagged(struct exp *list, const char *symbol) {
  return IS(list, PAIR) && exp_symbol_eq(CAR(list), symbol);
}

int exp_list_proper(struct exp *list) {
  struct exp *a = list;
  struct exp *b = list;
#define NEXT(x)                                 \
  do {                                          \
    if (x == NIL) {                             \
      return 1;                                 \
    } else if (!IS(x, PAIR)) {                  \
      return 0;                                 \
    }                                           \
    x = CDR(x);                                 \
  } while (0)
  for (;;) {
    NEXT(a);
    NEXT(b);
    NEXT(b);
    if (a == b) {
      return 0;
    }
  }
#undef NEXT
}

struct exp *exp_list_map(struct exp *list,
                         struct exp *(*fn)(struct exp *list,
                                           void *data),
                         void *data) {
  if (list == NIL) {
    return NIL;
  } else {
    struct exp *first = (*fn)(CAR(list), data);
    struct exp *rest = exp_list_map(CDR(list), fn, data);
    return exp_make_pair(first, rest);
  }
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
      struct strbuf *buf = strbuf_new(0);
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
  case CHARACTER:
    if (isgraph(exp->value.character)) {
      str = malloc(4);
      sprintf(str, "#\\%c", exp->value.character);
      return str;
    } else {
      const char *name = exp_char_to_name(exp->value.character);
      if (name != NULL) {
        str = malloc(strlen(name) + 3);
        sprintf(str, "#\\%s", name);
        return str;
      } else {
        str = malloc(6);
        sprintf(str, "#\\x%02x", exp->value.character);
        return str;
      }
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
    if (exp_symbol_eq(CAR(exp), "quote")) {
      size_t cap = 2;
      str = malloc(cap);
      len = 0;
      str[len] = '\0';
      CAT("'");
      char *s = exp_stringify(CAR(CDR(exp)));
      CAT(s);
      free(s);
      return str;
    } else {
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
        if (IS(exp, PAIR)) {
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
    {
      size_t i;
      size_t cap = 1;
      str = malloc(cap);
      len = 0;
      str[len] = '\0';
      CAT("#(");
      for (i = 0; i < vector_length(exp->value.vector); i += 1) {
        char *s;
        if (i > 0) {
          CAT(" ");
        }
        s = exp_stringify(vector_get(exp->value.vector, i));
        CAT(s);
        free(s);
      }
      CAT(")");
      return str;
    }
  case FUNCTION:
    str = malloc(strlen(exp->value.function.name) + 14);
    sprintf(str, "#<procedure:%s>", exp->value.function.name);
    return str;
  case CLOSURE:
    if (exp->value.closure.name != NULL) {
      str = malloc(strlen(exp->value.closure.name) + 14);
      sprintf(str, "#<procedure:%s>", exp->value.closure.name);
    } else {
      str = malloc(13);
      strcpy(str, "#<procedure>");
    }
    return str;
  case UNDEFINED:
    str = malloc(13);
    strcpy(str, "#<undefined>");
    return str;
  default:
    printf("exp type %d\n", exp->type);
    return err_error("exp_stringify: bad exp type");
  }
}

#undef CAT
