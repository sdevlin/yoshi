#ifndef ERR_H
#define ERR_H
#include <setjmp.h>
extern jmp_buf err_env;
extern const char *err_msg;
#define err_init() (setjmp(err_env))
extern void *err_ensure(int test, const char *msg);
extern void *err_error(const char *msg);
#endif
