// mini-CF: CFString == NSString. See minicf.h.
//
// The game's own CFStringRef literals are already NSString-shaped: src/objc.cpp
// rewrites every __cfstring in the loaded image into a GNUstep constant string, so the
// selectors below reach a real object whether the string came from the guest, from
// gnustep-base or from a mini-CF Create.
#include <pthread.h>
#include <string.h>

#include <Foundation/NSData.h>
#include <Foundation/NSDictionary.h>
#include <Foundation/NSString.h>
#include <common.h>

#include "minicf.h"

// CFStringEncoding and NSStringEncoding are different numbering schemes; only the
// encodings the games can actually name are mapped. 0 is "no such NSStringEncoding",
// which every caller below turns into a clean failure (NULL / false / no bytes) --
// which is also what CF does when it can't convert.
static NSStringEncoding ns_encoding(CFStringEncoding encoding) {
  switch (encoding) {
    case kCFStringEncodingMacRoman: return NSMacOSRomanStringEncoding;
    case kCFStringEncodingWindowsLatin1: return NSWindowsCP1252StringEncoding;
    case kCFStringEncodingISOLatin1: return NSISOLatin1StringEncoding;
    case kCFStringEncodingNextStepLatin: return NSNEXTSTEPStringEncoding;
    case kCFStringEncodingASCII: return NSASCIIStringEncoding;
    case kCFStringEncodingUTF8: return NSUTF8StringEncoding;
    case kCFStringEncodingNonLossyASCII: return NSNonLossyASCIIStringEncoding;
    case kCFStringEncodingUnicode: return NSUnicodeStringEncoding;  // == kCFStringEncodingUTF16
    case kCFStringEncodingUTF16BE: return NSUTF16BigEndianStringEncoding;
    case kCFStringEncodingUTF16LE: return NSUTF16LittleEndianStringEncoding;
    case kCFStringEncodingUTF32: return NSUTF32StringEncoding;
    case kCFStringEncodingUTF32BE: return NSUTF32BigEndianStringEncoding;
    case kCFStringEncodingUTF32LE: return NSUTF32LittleEndianStringEncoding;
    default: return 0;
  }
}

CFStringRef CFStringCreateWithBytes(CFAllocatorRef alloc, const UInt8 *bytes, CFIndex numBytes,
                                    CFStringEncoding encoding, Boolean isExternalRepresentation) {
  // isExternalRepresentation only says a BOM may be present on the UTF-16/32 encodings;
  // NSString's own decoders already handle a leading BOM, so it needs no separate path.
  NSStringEncoding const enc = ns_encoding(encoding);
  if (bytes == NULL || numBytes < 0 || enc == 0) return NULL;
  // Create rule: +1, and alloc/init avoids depending on an autorelease pool existing on
  // whichever game thread called us.
  return (CFStringRef)[[NSString alloc] initWithBytes:bytes length:(NSUInteger)numBytes encoding:enc];
}

CFStringRef CFStringCreateWithCString(CFAllocatorRef alloc, const char *cStr, CFStringEncoding encoding) {
  NSStringEncoding const enc = ns_encoding(encoding);
  if (cStr == NULL || enc == 0) return NULL;
  return (CFStringRef)[[NSString alloc] initWithBytes:cStr length:strlen(cStr) encoding:enc];
}

CFComparisonResult CFStringCompare(CFStringRef theString1, CFStringRef theString2,
                                   CFStringCompareFlags compareOptions) {
  if (theString1 == NULL || theString2 == NULL) {
    // CF has no defined answer here; order NULL first and don't crash.
    if (theString1 == theString2) return kCFCompareEqualTo;
    return theString1 == NULL ? kCFCompareLessThan : kCFCompareGreaterThan;
  }
  // The three flags that carry over 1:1 in value as well as meaning. Deliberately not
  // translated: kCFCompareNonliteral/kCFCompareLocalized/kCFCompareNumerically --
  // NSLiteralSearch is the *inverse* of kCFCompareNonliteral, and mapping it wrongly
  // would silently change orderings. Unmapped flags are dropped, which yields NSString's
  // default (non-literal, unlocalised) comparison.
  NSUInteger options = 0;
  if (compareOptions & kCFCompareCaseInsensitive) options |= NSCaseInsensitiveSearch;
  if (compareOptions & kCFCompareBackwards) options |= NSBackwardsSearch;
  if (compareOptions & kCFCompareAnchored) options |= NSAnchoredSearch;
  // NSComparisonResult and CFComparisonResult are both -1/0/1.
  return (CFComparisonResult)[(NSString *)theString1 compare:(NSString *)theString2 options:options];
}

Boolean CFStringGetCString(CFStringRef theString, char *buffer, CFIndex bufferSize, CFStringEncoding encoding) {
  NSStringEncoding const enc = ns_encoding(encoding);
  if (theString == NULL || buffer == NULL || bufferSize <= 0 || enc == 0) return false;
  // -getCString:maxLength:encoding: counts the NUL in maxLength, same as CF's bufferSize,
  // and returns NO (like CF's false) when the string doesn't fit.
  return [(NSString *)theString getCString:buffer maxLength:(NSUInteger)bufferSize encoding:enc] ? true : false;
}

// CFStringGetCStringPtr must hand back a pointer that stays valid for as long as the
// string does -- CF returns the string's own storage or NULL. gnustep's -UTF8String
// returns a *fresh autoreleased* buffer on every call (verified: consecutive calls on
// one GSTinyString return different pointers), which is neither stable nor safely
// ownable, and NULL is only safe if every caller takes the documented
// fall-back-to-CFStringGetCString path -- not something to bet a game on. So memoise:
// one retained UTF-8 NSData per distinct string value, keyed by the string itself.
// Bounded by the number of distinct strings the game asks about.
static NSMutableDictionary *cstring_cache;
static pthread_mutex_t cstring_cache_lock = PTHREAD_MUTEX_INITIALIZER;

const char *CFStringGetCStringPtr(CFStringRef theString, CFStringEncoding encoding) {
  if (theString == NULL) return NULL;
  // Only the byte-oriented ASCII-superset encodings can be answered with a C pointer.
  if (encoding != kCFStringEncodingUTF8 && encoding != kCFStringEncodingASCII &&
      encoding != kCFStringEncodingMacRoman)
    return NULL;

  NSString *str = (NSString *)theString;
  char const *utf8 = [str UTF8String];
  if (utf8 == NULL) return NULL;
  if (encoding != kCFStringEncodingUTF8) {
    // ASCII/MacRoman only match UTF-8 byte-for-byte while the string is ASCII.
    for (char const *p = utf8; *p; ++p)
      if ((unsigned char)*p >= 0x80) return NULL;
  }

  pthread_mutex_lock(&cstring_cache_lock);
  if (cstring_cache == nil) cstring_cache = [[NSMutableDictionary alloc] init];
  NSData *cached = [cstring_cache objectForKey:str];
  if (cached == nil) {
    cached = [NSData dataWithBytes:utf8 length:strlen(utf8) + 1];
    [cstring_cache setObject:cached forKey:str];
  }
  char const *bytes = (char const *)[cached bytes];
  pthread_mutex_unlock(&cstring_cache_lock);
  return bytes;
}
