#ifndef VECTOR_H
#define VECTOR_H
extern struct vector *vector_new(size_t hint);
extern void vector_free(struct vector **vp, void (*item_free)(void *item));
extern int vector_empty(struct vector *v);
extern size_t vector_length(struct vector *v);
extern void vector_resize(struct vector *v, size_t length,
                          void (*item_free)(void *item));
extern void *vector_get(struct vector *v, size_t index);
extern void vector_put(struct vector *v, size_t index, void *item);
extern void vector_push(struct vector *v, void *item);
extern void *vector_pop(struct vector *v);
extern void *vector_peek(struct vector *v);
extern void vector_each(struct vector *v,
                        void (*item_each)(void *item, void *data),
                        void *data);
extern struct vector *vector_map(struct vector *v,
                                 void *(item_map)(void *item, void *data),
                                 void *data);
extern struct vector *vector_filter(struct vector *v,
                                    int (*item_filter)(void *item, void *data),
                                    void *data);
#endif
