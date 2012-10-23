#ifndef INPUT_H
#define INPUT_H
struct input {
  int (*get)(struct input *self);
  void (*unget)(struct input *self, int c);
  int (*is_stdin)(struct input *self);
  void (*free)(struct input *self);
};
#endif
