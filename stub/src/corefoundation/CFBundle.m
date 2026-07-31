#include <common.h>
#include <Foundation/NSBundle.h>
#include <Foundation/NSURL.h>
#include "CoreFoundation/CFBundle.h"

// minicf.h last: it re-declares the shadowed CF entry points, so the CF calls below
// land on mini-CF (ObjC lifetime) rather than on corebase.
#include "minicf.h"

// CFBundle on NSBundle: a CFBundleRef *is* an NSBundle * (same rule as every other
// mini-CF type). This whole group moved together because the game round-trips the
// CFBundleRef it gets from CFBundleGetMainBundle back into the other CFBundle
// entry points -- mixing corebase's CFBundle objects with NSBundle-backed ones
// would walk one or the other into garbage.

CFBundleRef CFBundleGetMainBundle(void) {
  return (CFBundleRef)[NSBundle mainBundle];  // Get rule: +0
}

CFStringRef CFBundleGetIdentifier(CFBundleRef bundle) {
  if (bundle == NULL) return NULL;
  return (CFStringRef)[(NSBundle *)bundle bundleIdentifier];  // +0
}

CFBundleRef CFBundleCreate(CFAllocatorRef allocator, CFURLRef bundleURL) {
  if (bundleURL == NULL) return NULL;
  NSString *path = [(NSURL *)bundleURL path];
  if (path == nil) return NULL;
  return (CFBundleRef)[[NSBundle alloc] initWithPath:path];  // Create rule: +1
}

CFURLRef CFBundleCopyBundleURL(CFBundleRef bundle) {
  if (bundle == NULL) return NULL;
  NSURL *url = [NSURL fileURLWithPath:[(NSBundle *)bundle bundlePath]];
  return url ? (CFURLRef)[url retain] : NULL;  // Copy rule: +1
}

CFDictionaryRef CFBundleGetInfoDictionary(CFBundleRef bundle) {
  if (bundle == NULL) return NULL;
  return (CFDictionaryRef)[(NSBundle *)bundle infoDictionary];  // +0
}

CFArrayRef CFBundleCopyPreferredLocalizationsFromArray(CFArrayRef locArray) {
  // "Which of these localizations should I use?" NULL means "no preference" and the
  // caller falls back to its own default language.
  return NULL;
}

CFArrayRef CFBundleGetAllBundles(void) {
  return NULL;
}

CFDictionaryRef CFBundleCopyInfoDictionaryForURL(CFURLRef url) {
  if (url == NULL) return NULL;
  CFBundleRef bundle = CFBundleCreate(NULL, url);
  if (bundle == NULL) return NULL;
  CFDictionaryRef info = CFBundleGetInfoDictionary(bundle);
  if (info != NULL) CFRetain(info);
  CFRelease(bundle);
  return info;
}

CFURLRef CFBundleCopySharedSupportURL(CFBundleRef bundle) {
  // WC3 *does* ship Contents/SharedSupport (Blizzard Error.app), and it does NOT null-check
  // the result -- it feeds the URL straight into CFURLGetFileSystemRepresentation and then
  // CFReleases it, so returning NULL crashes in CFRelease(NULL). Build the real URL as
  // <bundle>/Contents/SharedSupport; every URL in the process stays an NSURL.
  if (bundle == NULL) return NULL;
  CFURLRef bundleURL = CFBundleCopyBundleURL(bundle);
  if (bundleURL == NULL) return NULL;
  NSURL *shared =
      [[(NSURL *)bundleURL URLByAppendingPathComponent:@"Contents"] URLByAppendingPathComponent:@"SharedSupport"];
  CFRelease(bundleURL);
  return shared ? (CFURLRef)[shared retain] : NULL;
}

uint32_t CFBundleGetVersionNumber(CFBundleRef bundle) {
  // The packed 'vers' number; the game only compares it against zero.
  return 0;
}
