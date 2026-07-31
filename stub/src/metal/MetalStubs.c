#include <common.h>
#include <string.h>

// Metal.framework's entire C surface: one function. Everything else the game uses is
// ObjC protocol methods discovered at runtime — see docs/METAL-NULL.md.

extern void *macholo_metal_null_device(void);

// Opt-in, and deliberately so. 16.3 weak-links Metal and picks its OpenGL path *because*
// this returns NULL; making a device appear unconditionally would silently move a working
// version onto the null renderer and turn its gate into a black screen. Read once.
//
//   unset / "" / "0"  -> NULL, exactly as before Metal HLE existed
//   "null"            -> the null renderer's device
static int metal_mode(void) {
  static int mode = -1;
  if (mode < 0) {
    const char *e = getenv("MACHOLO_METAL");
    mode = (e && strcmp(e, "null") == 0) ? 1 : 0;
    if (mode == 0 && e && *e && strcmp(e, "0") != 0)
      fprintf(stderr, "[metal] unknown MACHOLO_METAL=\"%s\", staying disabled\n", e);
  }
  return mode;
}

void *MTLCreateSystemDefaultDevice(void) {
  // Returning nil is what a real Mac with no supported GPU does, and it is what lets the
  // caller take its own "failed to create device" path instead of dying here.
  if (!metal_mode()) return NULL;
  return macholo_metal_null_device();
}
