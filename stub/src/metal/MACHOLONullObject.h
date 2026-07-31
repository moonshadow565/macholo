#pragma once
#import <Foundation/NSObject.h>
#import <Foundation/NSString.h>
#include "MetalTypes.h"

// Common base for every object the null Metal renderer hands to the game.
//
// The statically visible Metal surface is 8 symbols; everything else is protocol methods
// discovered at runtime. Rather than crash on each one in turn, unknown selectors are
// resolved into a generic property bag:
//
//   -setFoo:  stores a bare 64-bit word
//   -foo      returns the word previously stored for "foo" (0 if never set)
//   anything else returns 0/nil
//
// Every distinct selector resolved this way is logged once to stderr as
// "[metal-surface] -[Class selector]". That log *is* the discovery mechanism: a
// selector that shows up there and matters gets promoted to a real implementation, and
// the accumulated list is the spec for a future non-null Metal backend.
//
// Limits of the fallback, all of which mean "implement the method for real": a method
// returning float/double reads a garbage SSE register, and a method returning a large
// struct goes through objc_msgSend_stret whose hidden return pointer we do not honour.
@interface MACHOLONullObject : NSObject
@property(nonatomic, retain) NSString *label;

// Property-bag access, so hand-written methods can share storage with resolved ones.
- (uintptr_t)macholoValueForKey:(const char *)key;
- (void)macholoSetValue:(uintptr_t)value forKey:(const char *)key;
@end

// Log a selector we do not implement, once per (class, selector) pair.
void macholo_metal_note(const char *cls, const char *sel, char kind);
