#ifndef GC_H
#define GC_H
extern void gc_collect(void);
extern struct exp *gc_alloc_exp(enum exp_type type);
extern struct env *gc_alloc_env(struct env *parent);
#endif
