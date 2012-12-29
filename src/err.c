#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "err.h"
#include "exp.h"

jmp_buf err_env;
char *err_msg;

void err_cleanup(void) {
  free(err_msg);
  err_msg = NULL;
}

char *err_message(void) {
  if (err_msg != NULL) {
    char *msg = malloc(strlen(err_msg) + 1);
    strcpy(msg, err_msg);
    return msg;
  } else {
    return NULL;
  }
}

void *err_ensure(int test, const char *msg, struct exp *exp) {
  if (!test) {
    if (exp != NULL) {
      char *str = exp_stringify(exp);
      err_msg = malloc(strlen(msg) + strlen(str) + 3);
      sprintf(err_msg, "%s: %s", msg, str);
      free(str);
    } else {
      err_msg = malloc(strlen(msg) + 1);
      strcpy(err_msg, msg);
    }
    longjmp(err_env, 1);
  }
  return NULL;
}

void *err_error(const char *msg, struct exp *exp) {
  return err_ensure(0, msg, exp);
}
