// mini-CF: the small types -- CFBoolean, CFUUID, CFPropertyList. See minicf.h.
#include <Foundation/NSData.h>
#include <Foundation/NSPropertyList.h>
#include <Foundation/NSString.h>
#include <Foundation/NSUUID.h>
#include <Foundation/NSValue.h>
#include <common.h>

#include "minicf.h"

// CFBoolean == NSNumber's boolean singletons.
//
// These have to be the *same objects* gnustep's property-list parser produces for
// <true/> and <false/>, because the way CF code tests a plist flag is a pointer
// comparison against kCFBooleanTrue. Verified: +[NSNumber numberWithBool:] returns a
// per-value singleton (class NSBoolNumber) and NSPropertyListSerialization hands back
// that exact object, so CFPropertyListCreateFromXMLData below and these two constants
// have to move to mini-CF together -- which is why they share a commit.
//
// Deliberately not typed `const CFBooleanRef` the way CFNumber.h declares them: the
// value only exists once the ObjC runtime is up, so it is written by the constructor
// below, and a const global would be in relro and read-only by then.
CFBooleanRef kCFBooleanTrue DARWIN_SHADOW(kCFBooleanTrue) = NULL;
CFBooleanRef kCFBooleanFalse DARWIN_SHADOW(kCFBooleanFalse) = NULL;

// Runs after gnustep-base's own initialisation (libstub links it, so its constructors
// go first) and long before the loader maps the guest image, which is when the game's
// data import of kCFBooleanTrue is bound.
__attribute__((constructor)) static void minicf_boolean_init(void) {
  kCFBooleanTrue = (CFBooleanRef)[[NSNumber numberWithBool:YES] retain];
  kCFBooleanFalse = (CFBooleanRef)[[NSNumber numberWithBool:NO] retain];
}

CFUUIDRef CFUUIDCreate(CFAllocatorRef alloc) {
  // Create rule: +1. NSUUID's -init generates a random (version 4) UUID, same as CF.
  return (CFUUIDRef)[[NSUUID alloc] init];
}

CFUUIDBytes CFUUIDGetUUIDBytes(CFUUIDRef uuid) {
  // CFUUIDBytes is 16 UInt8 fields, i.e. the same 16 raw bytes -getUUIDBytes: writes.
  CFUUIDBytes bytes;
  memset(&bytes, 0, sizeof(bytes));
  if (uuid != NULL) [(NSUUID *)uuid getUUIDBytes:(unsigned char *)&bytes];
  return bytes;
}

CFPropertyListRef CFPropertyListCreateFromXMLData(CFAllocatorRef allocator, CFDataRef xmlData,
                                                  CFOptionFlags mutabilityOption, CFStringRef *errorString) {
  if (errorString != NULL) *errorString = NULL;
  if (xmlData == NULL) return NULL;

  // kCFPropertyListImmutable/MutableContainers/MutableContainersAndLeaves are 0/1/2,
  // and so are the NSPropertyListMutabilityOptions of the same names.
  NSString *error = nil;
  id plist = [NSPropertyListSerialization propertyListFromData:(NSData *)xmlData
                                              mutabilityOption:(NSPropertyListMutabilityOptions)mutabilityOption
                                                        format:NULL
                                              errorDescription:&error];
  if (plist == nil) {
    // CF's out-parameter is +1: the caller releases it.
    if (errorString != NULL && error != nil) *errorString = (CFStringRef)[error retain];
    return NULL;
  }
  // Create rule: +1.
  return (CFPropertyListRef)[plist retain];
}
