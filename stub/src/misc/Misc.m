#include <common.h>
#include <Foundation/NSArray.h>
#include <Foundation/NSBundle.h>
#include <Foundation/NSString.h>
#include <Foundation/NSURL.h>
#include "CoreFoundation/CFArray.h"
#include "CoreFoundation/CFBase.h"
#include "CoreFoundation/CFDictionary.h"
#include "CoreFoundation/CFString.h"
#include "CoreFoundation/CFBundle.h"
#include "CoreFoundation/CFURL.h"

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

// DATA: Security.framework attribute keys/values. On Apple these are CFStringRefs; the
// game only ever uses them as keys/values in the query dictionary it hands to
// SecItemCopyMatching (stubbed below), so toll-free-bridged constant NSStrings with the
// real Apple string values are enough.
CFStringRef kSecClass = (CFStringRef) @"class";
CFStringRef kSecClassCertificate = (CFStringRef) @"cert";
CFStringRef kSecMatchLimit = (CFStringRef) @"m_Limit";
CFStringRef kSecMatchLimitAll = (CFStringRef) @"m_LimitAll";
CFStringRef kSecMatchSearchList = (CFStringRef) @"m_SearchList";
CFStringRef kSecMatchTrustedOnly = (CFStringRef) @"m_TrustedOnly";
CFStringRef kSecReturnRef = (CFStringRef) @"r_Ref";

// typedef struct CF_BRIDGED_TYPE(id) __SecKeychain *SecKeychainRef;
// typedef struct CF_BRIDGED_TYPE(id) __SecCertificate *SecCertificateRef;
typedef CFArrayRef SecCertificateRef;
typedef CFArrayRef SecKeychainRef;

// CODE:
CFDataRef SecCertificateCopyData(SecCertificateRef certificate) {
  // https://github.com/curl/curl/blob/1ccf1cd9936dfa382fe1f061b639de20170bc8ea/lib/vtls/sectransp.c#L2850

  // FIXME: SecCertificateCopyData
  // panic("SecCertificateCopyData");
  return NULL;
}

OSStatus SecItemCopyMatching(CFDictionaryRef query, CFTypeRef *CF_RETURNS_RETAINED result) {
  // https://github.com/curl/curl/blob/1ccf1cd9936dfa382fe1f061b639de20170bc8ea/lib/vtls/sectransp.c#L1093
  // FIXME: SecItemCopyMatching
  // panic("SecItemCopyMatching");
  return 1;
}

OSStatus SecKeychainOpen(const char *pathName, SecKeychainRef *CF_RETURNS_RETAINED keychain) {
  // FIXME: SecKeychainOpen
  // panic("SecKeychainOpen");
  return 1;
}

CFDictionaryRef SCDynamicStoreCopyProxies() {
  // https://github.com/curl/curl/blob/1ccf1cd9936dfa382fe1f061b639de20170bc8ea/lib/macos.c#L47
  // FIXME: SCDynamicStoreCopyProxies
  // panic("SCDynamicStoreCopyProxies");
  return NULL;
}

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
  // Contents/SharedSupport is not part of this game's layout (it keeps everything under
  // Resources), and the caller treats a missing directory as "nothing to load".
  return NULL;
}

uint32_t CFBundleGetVersionNumber(CFBundleRef bundle) {
  // The packed 'vers' number; the game only compares it against zero.
  return 0;
}

void *MTLCreateSystemDefaultDevice(void) {
    // No Metal. Returning nil is what a real Mac with no supported GPU does, and it is what
    // lets the caller take its own "failed to create device" path instead of dying here.
    return NULL;
}
