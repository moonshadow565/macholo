// mini-CF: CFData == NSData. See minicf.h.
#include <Foundation/NSData.h>
#include <common.h>

#include "minicf.h"

CFDataRef CFDataCreate(CFAllocatorRef allocator, const UInt8 *bytes, CFIndex length) {
  // Create rule: +1. CFDataCreate copies the bytes (unlike CFDataCreateWithBytesNoCopy),
  // and so does -initWithBytes:length:.
  if (length < 0) return NULL;
  if (bytes == NULL && length != 0) return NULL;
  return (CFDataRef)[[NSData alloc] initWithBytes:bytes length:(NSUInteger)length];
}

const UInt8 *CFDataGetBytePtr(CFDataRef theData) {
  // Get rule: +0, and the pointer's lifetime is the data object's -- true for -bytes.
  if (theData == NULL) return NULL;
  return (const UInt8 *)[(NSData *)theData bytes];
}

CFIndex CFDataGetLength(CFDataRef theData) {
  if (theData == NULL) return 0;
  return (CFIndex)[(NSData *)theData length];
}
