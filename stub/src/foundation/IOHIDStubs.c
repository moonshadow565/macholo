#include <common.h>

// Neither CF nor NS surface (IOKit HID parameters / NX event status); kept in foundation/
// per the Misc.m dissolution's leftover bucket. See docs/REFACTOR.md Stage 2b report --
// this arguably belongs alongside src/iokit/stub_iokit.c's other IOHID mouse-accel stubs,
// but that regrouping is out of scope for this stage's pure code motion.

void IOHIDGetParameter() {
  // NOTE: used for mouse accell, we can safely stub it?
  panic("implement IOHIDGetParameter");
  return;
}

void IOHIDSetParameter() {
  // NOTE: used for mouse accell, we can safely stub it?
  panic("implement IOHIDSetParameter");
  return;
}

void NXCloseEventStatus(void* idk) {
  // NOTE: used for mouse accell, we can safely stub it?
  return;
}

void* NXOpenEventStatus() {
  // NOTE: used for mouse accell, we can safely stub it?
  // NOTE: if this returns NULL, other IOHID functions are not called.
  return NULL;
}
