#include <common.h>

// Neither CF nor NS surface (Metal.framework); kept in foundation/ per the Misc.m
// dissolution's leftover bucket. See docs/REFACTOR.md Stage 2b report.

void *MTLCreateSystemDefaultDevice(void) {
    // No Metal. Returning nil is what a real Mac with no supported GPU does, and it is what
    // lets the caller take its own "failed to create device" path instead of dying here.
    return NULL;
}
