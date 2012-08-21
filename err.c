#include <setjmp.h>
#include <stdlib.h>

#include "err.h"

jmp_buf err_env;
const char *err_msg;

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
