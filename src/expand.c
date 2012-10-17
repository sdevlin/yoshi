#include <assert.h>
#include <stdlib.h>

#include "err.h"
#include "exp.h"

static struct exp *expand_quote(struct exp *exp);
static struct exp *expand_set(struct exp *exp);
static struct exp *expand_define(struct exp *exp);
static struct exp *expand_if(struct exp *exp);
static struct exp *expand_lambda(struct exp *exp);
static struct exp *expand_begin(struct exp *exp);
static struct exp *expand_cond(struct exp *exp);
static struct exp *expand_and(struct exp *exp);
static struct exp *expand_or(struct exp *exp);
static struct exp *expand_quasiquote(struct exp *exp);
static struct exp *map_expand(struct exp *exp, void *data);

struct tag_expand {
  const char *tag;
  struct exp *(*expand)(struct exp *exp);
};

static struct tag_expand tag_map[] = {
  { .tag = "quote", .expand = &expand_quote },
  { .tag = "set!", .expand = &expand_set },
  { .tag = "define", .expand = &expand_define },
  { .tag = "if", .expand = &expand_if },
  { .tag = "lambda", .expand = &expand_lambda },
  { .tag = "begin", .expand = &expand_begin },
  { .tag = "cond", .expand = &expand_cond },
  { .tag = "and", .expand = &expand_and },
  { .tag = "or", .expand = &expand_or },
  { .tag = "quasiquote", .expand = &expand_quasiquote }
};

#define NELEM(arr) ((sizeof arr) / (sizeof arr[0]))

struct exp *expand(struct exp *exp) {
  assert(exp != NULL);
  if (!IS(exp, PAIR) || exp == NIL) {
    return exp;
  }
  size_t i;
  for (i = 0; i < NELEM(tag_map); i += 1) {
    if (exp_list_tagged(exp, tag_map[i].tag)) {
      return (*tag_map[i].expand)(exp);
    }
  }
  return exp_list_map(exp, &map_expand, NULL);
}

#undef NELEM

static struct exp *expand_quote(struct exp *exp) {
  err_ensure(exp_list_length(exp) == 2, "expand: bad syntax in quote");
  return exp;
}

static struct exp *expand_set(struct exp *exp) {
  err_ensure(exp_list_length(exp) == 3, "expand: bad syntax in set!");
  struct exp *var = CADR(exp);
  err_ensure(IS(var, SYMBOL), "expand: bad syntax in set!");
  return exp_make_list(CAR(exp),
                       var,
                       expand(CADDR(exp)),
                       NULL);
}

static struct exp *expand_define(struct exp *exp) {
  size_t length = exp_list_length(exp);
  err_ensure(length >= 3, "expand: bad syntax in define");
  struct exp *id = CADR(exp);
  switch (id->type) {
  case SYMBOL:
    err_ensure(length == 3, "expand: bad syntax in define");
    return exp_make_list(CAR(exp),
                         id,
                         expand(CADDR(exp)),
                         NULL);
  case PAIR:
    {
      struct exp *args = CDR(id);
      while (args != NIL) {
        if (IS(args, PAIR)) {
          err_ensure(IS(CAR(args), SYMBOL),
                     "expand: bad syntax in define");
          args = CDR(args);
        } else if (IS(args, SYMBOL)) {
          break;
        } else {
          err_error("expand: bad syntax in define");
        }
      }
      args = CDR(id);
      id = CAR(id);
      return exp_make_list(CAR(exp),
                           id,
                           expand(exp_make_pair(exp_make_symbol("lambda"),
                                                exp_make_pair(args,
                                                              CDDR(exp)))),
                           NULL);
    }
  default:
    return err_error("expand: bad syntax in define");
  }
}

static struct exp *expand_if(struct exp *exp) {
  switch (exp_list_length(exp)) {
  case 3:
    return exp_make_list(CAR(exp),
                         expand(CADR(exp)),
                         expand(CADDR(exp)),
                         OK,
                         NULL);
  case 4:
    return exp_make_pair(CAR(exp),
                         exp_list_map(CDR(exp), &map_expand, NULL));
  default:
    return err_error("expand: bad syntax in if");
  }
}

static struct exp *expand_lambda(struct exp *exp) {
  size_t length = exp_list_length(exp);
  err_ensure(length >= 3, "expand: bad syntax in lambda");
  struct exp *params = CADR(exp);
  switch (params->type) {
  case PAIR:
    {
      struct exp *param = params;
      while (param != NIL) {
        if (IS(param, PAIR)) {
          err_ensure(IS(CAR(param), SYMBOL), "expand: bad syntax in lambda");
          param = CDR(param);
        } else if (IS(param, SYMBOL)) {
          break;
        } else {
          err_error("expand: bad syntax in lambda");
        }
      }
    }
    break;
  case SYMBOL:
  case NIL_TYPE:
    break;
  default:
    return err_error("expand: bad syntax in lambda");
  }
  struct exp *body = (length == 3 ?
                      exp_make_pair(expand(CADDR(exp)), NIL) :
                      exp_make_pair(expand(exp_make_pair(exp_make_symbol("begin"),
                                                         CDDR(exp))), NIL));
  return exp_make_pair(CAR(exp),
                       exp_make_pair(params, body));
}

/* doing this right requires a bunch of context */
/* because there are different kinds of `begin` */
/* punting for now */
static struct exp *expand_begin(struct exp *exp) {
  err_ensure(exp_list_length(exp) >= 2, "expand: bad syntax in begin");
  return exp_make_pair(CAR(exp), exp_list_map(CDR(exp), &map_expand, NULL));
}

static struct exp *expand_cond_clauses(struct exp *exp) {
  if (exp == NIL) {
    return OK;
  } else {
    struct exp *clause = CAR(exp);
    /* the standard specifies other valid clause forms */
    /* these are not supported as yet */
    err_ensure(exp_list_length(clause) == 2, "expand: bad syntax in cond");
    struct exp *pred = CAR(clause);
    return exp_make_list(exp_make_atom("if"),
                         exp_symbol_eq(pred, "else") ? TRUE : expand(pred),
                         expand(CADR(clause)),
                         expand_cond_clauses(CDR(exp)),
                         NULL);
  }
}

static struct exp *expand_cond(struct exp *exp) {
  err_ensure(exp_list_length(exp) >= 2, "expand: bad syntax in cond");
  return expand_cond_clauses(CDR(exp));
}

static struct exp *expand_and_tests(struct exp *exp) {
  size_t length = exp_list_length(exp);
  switch (length) {
  case 0:
    return TRUE;
  case 1:
    return CAR(exp);
  default:
    return exp_make_list(exp_make_atom("if"),
                         CAR(exp),
                         expand_and_tests(CDR(exp)),
                         FALSE,
                         NULL);
  }
}

static struct exp *expand_and(struct exp *exp) {
  err_ensure(exp_list_proper(exp), "expand: bad syntax in and");
  return expand_and_tests(exp_list_map(CDR(exp), &map_expand, NULL));
}

/* need some kind of hygienic macros to do this properly */
static struct exp *expand_or(struct exp *exp) {
  return exp_make_pair(CAR(exp), exp_list_map(CDR(exp), &map_expand, NULL));
}

static struct exp *expand_quasiquote(struct exp *exp) {
  return exp;
}

static struct exp *map_expand(struct exp *exp, void *data) {
  return expand(exp);
}
