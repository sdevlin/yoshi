#include <stdio.h>
#include <stdlib.h>

#include "exp.h"
#include "print.h"

void print(struct exp *exp) {
  switch (exp->type) {
  case UNDEFINED:
    break;
  default:
    {
      char *str = exp_stringify(exp);
      printf("%s\n", str);
      free(str);
    }
    break;
  }
}
