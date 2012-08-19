#ifndef ERR_H
#define ERR_H
extern const char *err_msg;
extern int err_init(void);
extern void *err_ensure(int test, const char *msg);
extern void *err_error(const char *msg);
#endif
