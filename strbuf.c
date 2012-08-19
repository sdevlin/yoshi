#include <stdlib.h>
#include <string.h>

#include "strbuf.h"

struct strbuf {
  size_t len;
  size_t cap;
  char *buf;
};

struct strbuf *strbuf_make(size_t cap) {
  struct strbuf *buf = malloc(sizeof *buf);
  buf->len = 0;
  buf->cap = cap > 0 ? cap : 8;
  buf->buf = malloc(buf->cap);
  return buf;
}

void strbuf_free(struct strbuf *buf) {
  free(buf->buf);
  free(buf);
}

void strbuf_push(struct strbuf *buf, char c) {
  if (buf->len == buf->cap) {
    buf->cap *= 2;
    buf->buf = realloc(buf->buf, buf->cap);
  }
  buf->buf[buf->len] = c;
  buf->len += 1;
}

char *strbuf_to_cstr(struct strbuf *buf) {
  char *str = malloc(buf->len + 1);
  strncpy(str, buf->buf, buf->len);
  str[buf->len] = '\0';
  return str;
}
