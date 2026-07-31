#include <common.h>
#include <Foundation/NSURL.h>
#include "CoreFoundation/CFArray.h"
#include "CoreFoundation/CFBase.h"
#include "CoreFoundation/CFBundle.h"
#include "CoreFoundation/CFDictionary.h"
#include "CoreFoundation/CFURL.h"

// DATA:
// NSImageNameStopProgressFreestandingTemplate

// CFBundle gaps. gnustep-corebase implements most of the CFBundle API (including
// CFBundleGetMainBundle/CFBundleGetIdentifier/CFBundleCopyBundleURL), so those must NOT
// be shadowed here -- the game passes the CFBundleRef it gets back into corebase's own
// functions, and an NSBundle in its place walks straight into garbage. Only the few
// entry points corebase is missing live here.
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
  // <bundle>/Contents/SharedSupport. corebase's CFBundleCopyBundleURL hands back an NSURL
  // (see CFURL.m), so append with NSURL to keep every URL in the process an NSURL.
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
