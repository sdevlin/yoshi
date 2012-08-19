#include <setjmp.h>
#include <stdlib.h>

#include "err.h"

static jmp_buf err_env;
const char *err_msg;

int err_init(void) {
  return setjmp(err_env);
}

void *err_ensure(int test, const char *msg) {
  if (!test) {
    err_msg = msg;
    longjmp(err_env, 1);
  }
  return NULL;
}

void *err_error(const char *msg) {
  return err_ensure(0, msg);
}
