#ifndef EXP_H
#define EXP_H
enum exp_type {
  UNDEFINED,
  PAIR,
  FIXNUM,
  BOOLEAN,
  SYMBOL,
  STRING,
  VECTOR,
  CLOSURE,
  FUNCTION,
  NIL_TYPE
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
    struct {
      size_t len;
      struct exp **items;
    } vector;
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

extern struct exp *exp_make_atom(char *str);
extern struct exp *exp_make_list(struct exp *first, ...);
extern struct exp *exp_make_string(char *str);
extern struct exp *exp_make_pair(struct exp *first, struct exp *rest);
extern struct exp *exp_make_fixnum(long fixnum);
struct env;
extern struct exp *exp_make_closure(struct exp *params, struct exp *body,
                                    struct env *env);
extern int exp_symbol_eq(struct exp *exp, const char *s);
extern size_t exp_list_length(struct exp *list);
#define CAR(exp) (exp->value.pair.first)
#define CDR(exp) (exp->value.pair.rest)
extern struct exp *exp_nth(struct exp *list, size_t n);
extern char *exp_stringify(struct exp *exp);
#endif
