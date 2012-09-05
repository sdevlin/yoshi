#include <assert.h>
#include <stdlib.h>

#include "vector.h"

struct vector {
  size_t length;
  size_t capacity;
  void **items;
};

static void vector_reserve(struct vector *v, size_t capacity) {
  assert(v != NULL);
  v->capacity = capacity;
  v->items = realloc(v->items, capacity * sizeof *v->items);
}

struct vector *vector_new(size_t hint) {
  struct vector *v = calloc(1, sizeof *v);
  vector_reserve(v, hint > 0 ? hint : 1);
  return v;
}

void vector_free(struct vector **vp, void (*item_free)(void *item)) {
  struct vector *v;
  assert(*vp);
  v = *vp;
  *vp = NULL;
  if (item_free != NULL) {
    size_t i;
    for (i = 0; i < v->length; i += 1) {
      (*item_free)(*(v->items + i));
    }
  }
  free(v->items);
  free(v);
}

int vector_empty(struct vector *v) {
  assert(v != NULL);
  return v->length == 0;
}

size_t vector_length(struct vector *v) {
  assert(v != NULL);
  return v->length;
}

static void vector_shrink(struct vector *v, size_t length,
                          void (*item_free)(void *item)) {
  assert(v != NULL);
  assert(length < v->length);
  if (item_free != NULL) {
    size_t i;
    for (i = length; i < v->length; i += 1) {
      (*item_free)(*(v->items + i));
    }
  }
  v->length = length;
}

static void vector_grow(struct vector *v, size_t length) {
  assert(v != NULL);
  assert(length > v->length);
  if (v->capacity < length) {
    size_t capacity = v->capacity;
    do {
      capacity *= 2;
    } while (capacity < length);
    vector_reserve(v, capacity);
  }
  v->length = length;
}

void vector_resize(struct vector *v, size_t length,
                   void (*item_free)(void *item)) {
  assert(v != NULL);
  if (length < v->length) {
    vector_shrink(v, length, item_free);
  } else if (length > v->length) {
    vector_grow(v, length);
  }
}

void *vector_get(struct vector *v, size_t index) {
  assert(v != NULL);
  assert(index < v->length);
  return *(v->items + index);
}

void vector_put(struct vector *v, size_t index, void *item) {
  assert(v != NULL);
  assert(index < v->length);
  *(v->items + index) = item;
}

void vector_push(struct vector *v, void *item) {
  assert(v != NULL);
  vector_resize(v, v->length + 1, NULL);
  vector_put(v, v->length - 1, item);
}

void *vector_pop(struct vector *v) {
  assert(v != NULL);
  assert(v->length > 0);
  void *item = vector_peek(v);
  vector_resize(v, v->length - 1, NULL);
  return item;
}

void *vector_peek(struct vector *v) {
  assert(v != NULL);
  assert(v->length > 0);
  return *(v->items + v->length - 1);
}

void vector_each(struct vector *v,
                 void (*item_each)(void *item, void *data),
                 void *data) {
  assert(v != NULL);
  assert(item_each != NULL);
  size_t i;
  for (i = 0; i < v->length; i += 1) {
    (*item_each)(vector_get(v, i), data);
  }
}

struct vector *vector_map(struct vector *v,
                          void *(*item_map)(void *item, void *data),
                          void *data) {
  assert(v != NULL);
  assert(item_map != NULL);
  struct vector *new = vector_new(v->length);
  size_t i;
  for (i = 0; i < v->length; i += 1) {
    vector_push(new, (*item_map)(vector_get(v, i), data));
  }
  return new;
}

struct vector *vector_filter(struct vector *v,
                             int (*item_filter)(void *item, void *data),
                             void *data) {
  assert(v != NULL);
  assert(item_filter != NULL);
  struct vector *new = vector_new(0);
  size_t i;
  for (i = 0; i < v->length; i += 1) {
    void *item = vector_get(v, i);
    if ((*item_filter)(item, data)) {
      vector_push(new, item);
    }
  }
  return new;
}
