/* Frozen stand-in for the `Source/dynamic-load.h` that gnustep-base's
 * configure/config.status materialises.  On x86_64-linux-gnu configure picks
 * the dlopen()-based backend, i.e. it makes dynamic-load.h a copy of
 * Source/simple-load.h.  Forwarding keeps the vendored source authoritative.
 */
#include <simple-load.h>
