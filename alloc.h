#ifndef ALLOC_H
#define ALLOC_H
extern struct exp *alloc_exp(enum exp_type type);
extern struct env *alloc_env(struct env *parent);
#endif
