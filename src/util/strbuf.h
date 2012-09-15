#ifndef STRBUF_H
#define STRBUF_H
extern struct strbuf *strbuf_new(size_t cap);
extern void strbuf_free(struct strbuf *buf);
extern void strbuf_push(struct strbuf *buf, char c);
extern char *strbuf_to_cstr(struct strbuf *buf);
#endif
