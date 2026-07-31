// mini-CF: the allocator/lifetime base. See minicf.h for the design and the
// $darwin$ shadowing mechanism.
//
// CF lifetime is ObjC lifetime here: a CFTypeRef is an id, CFRetain/CFRelease are
// -retain/-release. That is exactly what corebase does *for objects it knows about*
// (its NSCF* bridge classes), but it also works for the CF-shaped objects the rest of
// the stub hands the game -- CGDisplayMode (coregraphics/CG.m) is a plain NSObject
// subclass, and every URL in the process is an NSURL (CFURL.m).
#include <Foundation/NSObject.h>
#include <Foundation/NSValue.h>
#include <common.h>

#include "minicf.h"

// CF's documented "default allocator", which is defined to be equivalent to NULL.
// mini-CF ignores allocators outright (NS objects come from the default zone), so the
// token only has to be something every mini-CF entry point is happy to receive.
CFAllocatorRef kCFAllocatorDefault = NULL;

// CFBoolean is a *constant* on Darwin: CFRetain/CFRelease on kCFBooleanTrue/False are
// no-ops there. gnustep-base's +numberWithBool: singletons are ordinary refcounted
// objects with a retain count of 1, so an unbalanced CFRelease from the game would
// free them out from under the process (verified: three releases and the next use
// segfaults). Treat them as immortal.
static Boolean is_immortal(CFTypeRef cf) {
  return cf == (CFTypeRef)kCFBooleanTrue || cf == (CFTypeRef)kCFBooleanFalse;
}

CFTypeRef CFRetain(CFTypeRef cf) {
  // Darwin traps on CFRetain(NULL); the stub's house style is to absorb it instead.
  if (cf == NULL || is_immortal(cf)) return cf;
  return (CFTypeRef)[(id)cf retain];
}

void CFRelease(CFTypeRef cf) {
  if (cf == NULL || is_immortal(cf)) return;
  [(id)cf release];
}

CFTypeRef CFAutorelease(CFTypeRef cf) {
  if (cf == NULL || is_immortal(cf)) return cf;
  return (CFTypeRef)[(id)cf autorelease];
}

CFTypeRef CFMakeCollectable(CFTypeRef cf) {
  // ObjC garbage collection is dead on Darwin too (and was never enabled for these
  // binaries); the real implementation is the identity outside a GC-enabled process.
  // 14.8 imports this, 16.3/16.15 no longer do.
  return cf;
}
