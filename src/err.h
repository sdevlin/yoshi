#ifndef ERR_H
#define ERR_H
#include <setjmp.h>
extern jmp_buf err_env;
#define err_init() (err_cleanup(), setjmp(err_env))
extern void err_cleanup(void);
extern char *err_message(void);
struct exp;
extern void *err_ensure(int test, const char *msg, struct exp *exp);
extern void *err_error(const char *msg, struct exp *exp);
#endif
