#ifndef GC_ALLOC_H
#define GC_ALLOC_H
extern struct exp *gc_alloc_exp(enum exp_type type);
extern struct env *gc_alloc_env(struct env *parent);
#endif
