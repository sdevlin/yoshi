#ifndef GC_H
#define GC_H
#include "exp.h"
struct gc {
  void (*init)(void);
  void (*collect)(void);
  struct exp *(*alloc_exp)(enum exp_type type);
  struct env *(*alloc_env)(struct env *parent);
};
extern struct gc gc_nop;
extern struct gc gc_ms;
extern struct gc gc_copy;
#endif
