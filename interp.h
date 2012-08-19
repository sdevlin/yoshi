#ifndef INTERP_H
#define INTERP_H
struct env;
extern struct exp *eval(struct exp *exp, struct env *env);
extern struct exp *apply(struct exp *fn, struct exp *args,
                         struct env *env);
#endif
