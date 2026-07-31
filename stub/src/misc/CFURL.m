// CFURL, redirected onto Foundation's NSURL.
//
// gnustep-corebase ships two incompatible halves: CFBundle.m is a thin shim over
// gnustep-base (CFBundleCopyBundleURL hands back a real NSURL object), while CFURL.c is
// a standalone CFRuntime implementation that dereferences its own struct. Feed one to
// the other -- which is exactly what the game does, CFBundleCopyBundleURL straight into
// CFURLGetFileSystemRepresentation -- and it reads a NULL string pointer and crashes.
//
// The $darwin$ prefix makes lib::resolve prefer these over corebase's, so every URL in
// the process is an NSURL and the two halves agree.
#include <string.h>

#include <Foundation/NSString.h>
#include <Foundation/NSURL.h>
#include <common.h>

#include "CoreFoundation/CFBase.h"
#include "CoreFoundation/CFString.h"
#include "CoreFoundation/CFURL.h"

// GNUstep bridges CFString to NSString (see the __cfstring conversion in src/objc.cpp),
// so a CFStringRef can be used as an NSString directly.
CFURLRef $darwin$CFURLCreateWithFileSystemPath(CFAllocatorRef allocator, CFStringRef filePath, int pathStyle,
                                               Boolean isDirectory) {
  if (filePath == NULL) return NULL;
  return (CFURLRef)[[NSURL fileURLWithPath:(NSString *)filePath isDirectory:isDirectory] retain];
}

CFURLRef $darwin$CFURLCreateFromFileSystemRepresentation(CFAllocatorRef allocator, const uint8_t *buffer,
                                                         CFIndex bufLen, Boolean isDirectory) {
  if (buffer == NULL) return NULL;
  NSString *path = [[[NSString alloc] initWithBytes:buffer length:(NSUInteger)bufLen
                                           encoding:NSUTF8StringEncoding] autorelease];
  if (path == Nil) return NULL;
  return (CFURLRef)[[NSURL fileURLWithPath:path isDirectory:isDirectory] retain];
}

CFURLRef $darwin$CFURLCreateWithString(CFAllocatorRef allocator, CFStringRef URLString, CFURLRef baseURL) {
  if (URLString == NULL) return NULL;
  return (CFURLRef)[[NSURL URLWithString:(NSString *)URLString relativeToURL:(NSURL *)baseURL] retain];
}

Boolean $darwin$CFURLGetFileSystemRepresentation(CFURLRef url, Boolean resolveAgainstBase, uint8_t *buffer,
                                                 CFIndex maxBufLen) {
  if (url == NULL || buffer == NULL || maxBufLen <= 0) return false;
  NSURL *nsurl = (NSURL *)url;
  NSString *path = resolveAgainstBase ? [[nsurl absoluteURL] path] : [nsurl path];
  if (path == Nil) return false;

  char const *utf8 = [path UTF8String];
  if (utf8 == NULL) return false;
  size_t const n = strlen(utf8);
  if (n + 1 > (size_t)maxBufLen) return false;
  memcpy(buffer, utf8, n + 1);
  return true;
}
