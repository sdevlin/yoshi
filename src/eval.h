#ifndef EVAL_H
#define EVAL_H
struct env;
extern struct exp *eval(struct exp *exp, struct env *env);
#endif
