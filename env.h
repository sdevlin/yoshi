#ifndef ENV_H
#define ENV_H
struct env;
extern struct exp *env_define(struct env *env, struct exp *symbol,
                              struct exp *value);
extern struct exp *env_lookup(struct env *env, struct exp *symbol);
extern struct exp *env_update(struct env *env, struct exp *symbol,
                              struct exp *value);
#endif
