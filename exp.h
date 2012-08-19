#ifndef EXP_H
#define EXP_H
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
