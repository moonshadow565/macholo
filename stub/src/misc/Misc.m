#include <common.h>
#include <Foundation/NSString.h>
#include "CoreFoundation/CFArray.h"
#include "CoreFoundation/CFBase.h"
#include "CoreFoundation/CFDictionary.h"
#include "CoreFoundation/CFString.h"

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

void CFBundleCopyPreferredLocalizationsFromArray() {
  // FIXME: CFBundleCopyPreferredLocalizationsFromArray
  panic("CFBundleCopyPreferredLocalizationsFromArray");
}

void CFBundleGetIdentifier() {
  // FIXME: CFBundleGetIdentifier
  panic("CFBundleGetIdentifier");
}

void CFBundleGetMainBundle() {
  // FIXME: CFBundleGetMainBundle
  panic("CFBundleGetMainBundle");
}

void MTLCreateSystemDefaultDevice() {
    // NOTE: should only be used by metal, dont' care
    panic("MTLCreateSystemDefaultDevice");
}
