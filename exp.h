#ifndef EXP_H
#define EXP_H
#define CAR(exp) (exp->value.pair.first)
#define CDR(exp) (exp->value.pair.rest)
extern struct exp *exp_nth(struct exp *list, size_t n);
extern char *exp_stringify(struct exp *exp);
#endif
