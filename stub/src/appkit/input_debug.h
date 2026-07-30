#pragma once
// Opt-in tracing for the SDL -> NSEvent -> responder-chain path.
// Enable with MACHOLO_INPUT_DEBUG=1 in the environment; off (and nearly free) otherwise.
#include <stdio.h>
#include <stdlib.h>
#ifdef __OBJC__
#include <objc/runtime.h>
#endif

static inline int input_debug_enabled(void) {
  static int state = -1;
  if (state < 0) {
    char const *v = getenv("MACHOLO_INPUT_DEBUG");
    state = (v && *v && *v != '0') ? 1 : 0;
  }
  return state;
}

#define INPUT_DEBUG(fmt, ...)                                                  \
  do {                                                                         \
    if (input_debug_enabled()) {                                               \
      fprintf(stderr, "[input] " fmt "\n" __VA_OPT__(, ) __VA_ARGS__);         \
      fflush(stderr);                                                          \
    }                                                                          \
  } while (0)
