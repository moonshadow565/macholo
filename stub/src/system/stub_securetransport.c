// SecureTransport (SSL*) plus the certificate-inspection and code-signing corners of
// Security.framework. The menu does not need the network, and there is no keychain or
// trust store behind any of this, so every entry point fails cleanly: SSLCreateContext
// hands back nothing and the rest report errSSLInternal / errSecInternal, which is a
// path the callers already handle (they show "could not connect" and carry on).
#include <stdint.h>

#include <common.h>

typedef int32_t OSStatus;
typedef const void *CFTypeRef;
typedef const void *CFStringRef;
typedef const void *CFArrayRef;
typedef const void *CFDataRef;
typedef const void *CFDateRef;
typedef const void *CFDictionaryRef;
typedef const void *CFErrorRef;
typedef const void *CFURLRef;
typedef const void *CFAllocatorRef;
typedef void *SSLContextRef;
typedef void *SecTrustRef;
typedef void *SecCertificateRef;
typedef void *SecKeyRef;
typedef void *SecCodeRef;
typedef void *SecStaticCodeRef;

enum {
  errSSLInternal = -9810,
  errSSLClosedAbort = -9806,
  errSecInternal = -26276,
  errSecItemNotFound = -25300,
};

// ---------------------------------------------------------------------------
// SecureTransport
// ---------------------------------------------------------------------------

SSLContextRef SSLCreateContext(CFAllocatorRef alloc, int protocolSide, int connectionType) {
  return NULL;
}

OSStatus SSLSetIOFuncs(SSLContextRef ctx, void *readFunc, void *writeFunc) {
  return errSSLInternal;
}

OSStatus SSLSetConnection(SSLContextRef ctx, const void *connection) {
  return errSSLInternal;
}

OSStatus SSLSetProtocolVersionMin(SSLContextRef ctx, int minVersion) {
  return errSSLInternal;
}

OSStatus SSLSetProtocolVersionMax(SSLContextRef ctx, int maxVersion) {
  return errSSLInternal;
}

OSStatus SSLSetSessionOption(SSLContextRef ctx, int option, unsigned char value) {
  return errSSLInternal;
}

OSStatus SSLHandshake(SSLContextRef ctx) {
  return errSSLInternal;
}

OSStatus SSLGetSessionState(SSLContextRef ctx, int *state) {
  if (state) *state = 0;
  return errSSLInternal;
}

OSStatus SSLGetBufferedReadSize(SSLContextRef ctx, size_t *bufferSize) {
  if (bufferSize) *bufferSize = 0;
  return errSSLInternal;
}

OSStatus SSLRead(SSLContextRef ctx, void *data, size_t dataLength, size_t *processed) {
  if (processed) *processed = 0;
  return errSSLClosedAbort;
}

OSStatus SSLWrite(SSLContextRef ctx, const void *data, size_t dataLength, size_t *processed) {
  if (processed) *processed = 0;
  return errSSLClosedAbort;
}

OSStatus SSLCopyPeerTrust(SSLContextRef ctx, SecTrustRef *trust) {
  if (trust) *trust = NULL;
  return errSSLInternal;
}

// ---------------------------------------------------------------------------
// Trust evaluation / certificate inspection
// ---------------------------------------------------------------------------

OSStatus SecTrustEvaluate(SecTrustRef trust, int *result) {
  // kSecTrustResultInvalid. Nothing is trusted because nothing was ever verified.
  if (result) *result = 0;
  return errSecInternal;
}

OSStatus SecTrustSetAnchorCertificates(SecTrustRef trust, CFArrayRef anchorCertificates) {
  return errSecInternal;
}

OSStatus SecTrustSetAnchorCertificatesOnly(SecTrustRef trust, unsigned char anchorCertificatesOnly) {
  return errSecInternal;
}

OSStatus SecTrustSetVerifyDate(SecTrustRef trust, CFDateRef verifyDate) {
  return errSecInternal;
}

long SecTrustGetCertificateCount(SecTrustRef trust) {
  return 0;
}

SecCertificateRef SecTrustGetCertificateAtIndex(SecTrustRef trust, long ix) {
  return NULL;
}

SecCertificateRef SecCertificateCreateWithData(CFAllocatorRef allocator, CFDataRef data) {
  return NULL;
}

SecKeyRef SecCertificateCopyPublicKey(SecCertificateRef certificate) {
  return NULL;
}

CFDataRef SecCertificateCopySerialNumber(SecCertificateRef certificate, CFErrorRef *error) {
  return NULL;
}

CFDictionaryRef SecCertificateCopyValues(SecCertificateRef certificate, CFArrayRef keys, CFErrorRef *error) {
  return NULL;
}

CFDataRef SecKeyCopyExternalRepresentation(SecKeyRef key, CFErrorRef *error) {
  return NULL;
}

CFStringRef SecCopyErrorMessageString(OSStatus status, void *reserved) {
  return NULL;
}

// ---------------------------------------------------------------------------
// Code signing -- the game checks its own signature
// ---------------------------------------------------------------------------

OSStatus SecStaticCodeCreateWithPath(CFURLRef path, uint32_t flags, SecStaticCodeRef *staticCode) {
  if (staticCode) *staticCode = NULL;
  return errSecInternal;
}

OSStatus SecStaticCodeCheckValidityWithErrors(SecStaticCodeRef staticCode, uint32_t flags, void *requirement,
                                              CFErrorRef *errors) {
  if (errors) *errors = NULL;
  return errSecInternal;
}

OSStatus SecCodeCopySigningInformation(SecStaticCodeRef code, uint32_t flags, CFDictionaryRef *information) {
  if (information) *information = NULL;
  return errSecInternal;
}

// DATA: dictionary keys for the APIs above. Nothing ever gets far enough to use them
// as keys, but they must exist as distinct non-NULL values.
CFStringRef kSecCodeInfoTrust = (CFStringRef) "trust";
CFStringRef kSecPropertyKeyType = (CFStringRef) "type";
CFStringRef kSecPropertyKeyLabel = (CFStringRef) "label";
CFStringRef kSecPropertyKeyValue = (CFStringRef) "value";
CFStringRef kSecPropertyTypeSection = (CFStringRef) "section";
CFStringRef kSecPropertyTypeData = (CFStringRef) "data";
CFStringRef kSecOIDCommonName = (CFStringRef) "2.5.4.3";
CFStringRef kSecOIDCountryName = (CFStringRef) "2.5.4.6";
CFStringRef kSecOIDStateProvinceName = (CFStringRef) "2.5.4.8";
CFStringRef kSecOIDLocalityName = (CFStringRef) "2.5.4.7";
CFStringRef kSecOIDOrganizationName = (CFStringRef) "2.5.4.10";
CFStringRef kSecOIDOrganizationalUnitName = (CFStringRef) "2.5.4.11";
CFStringRef kSecOIDEmailAddress = (CFStringRef) "1.2.840.113549.1.9.1";
CFStringRef kSecOIDSubjectAltName = (CFStringRef) "2.5.29.17";
CFStringRef kSecOIDSubjectKeyIdentifier = (CFStringRef) "2.5.29.14";
CFStringRef kSecOIDAuthorityKeyIdentifier = (CFStringRef) "2.5.29.35";
CFStringRef kSecOIDX509V1IssuerName = (CFStringRef) "2.5.29.4";
CFStringRef kSecOIDX509V1SubjectName = (CFStringRef) "2.5.29.6";
CFStringRef kSecOIDX509V1SubjectPublicKey = (CFStringRef) "2.5.29.8";
