#ifndef CONFIG_H
#define CONFIG_H

#include "util/input.h"

enum flag_type {
  OFF,
  ON
};

struct flags {
  enum flag_type debug;
  enum flag_type interactive;
  enum flag_type silent;
};

extern struct flags config;

extern void config_init(int argc, char **argv);

extern struct input *config_next_input(void);

extern struct gc *gc;
#endif
