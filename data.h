#ifndef DATA_H
#define DATA_H
enum flag_type {
  OFF,
  ON
};

enum exp_type {
  UNDEFINED,
  PAIR,
  FIXNUM,
  SYMBOL,
  STRING,
  CLOSURE,
  FUNCTION,
  CONSTANT
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
    char *string;
    struct exp *(*function)(struct exp *args);
    struct {
      struct exp *params;
      struct exp *body;
      struct env *env;
    } closure;
  } value;
};

extern struct exp nil;
#define NIL (&nil)

extern struct exp ok;
#define OK (&ok)

extern struct exp true;
#define TRUE (&true)

extern struct exp false;
#define FALSE (&false)

struct env {
  struct binding {
    char *symbol;
    struct exp *value;
    struct binding *next;
  } *bindings;
  struct env *parent;
};
#endif
