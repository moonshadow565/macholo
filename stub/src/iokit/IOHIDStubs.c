#include <common.h>

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
