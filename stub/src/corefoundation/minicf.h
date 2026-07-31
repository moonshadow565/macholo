#pragma once
// mini-CF -- CoreFoundation implemented as ObjC calls into gnustep-base.
//
// docs/REFACTOR.md track 2 option B, scoped in docs/surface-audit/MINICF.md. A CF
// object here *is* an NS object: CFStringRef == NSString *, CFArrayRef == NSArray *,
// CFDictionaryRef == NSDictionary *, CFDataRef == NSData *, CFURLRef == NSURL *. Toll-
// free bridging is then true by construction instead of by gnustep-corebase's two
// halves agreeing (they don't -- see LOADER.md's CFURL-vs-CFBundle NULL deref).
//
// While corebase is still linked, mini-CF has to win over its plain exports. The
// loader's resolver does that by preferring a "$darwin$"-prefixed symbol
// (src/lib.cpp lib::resolve step 1). Rather than renaming every definition, each entry
// point is *defined under its real CF name* and only the emitted symbol is prefixed,
// via an asm label attached to the declarations below. Two consequences:
//
//   - the .m files contain plain `CFRetain(...)` definitions and plain CF call sites;
//   - unlinking corebase is one edit: make DARWIN_SHADOW expand to nothing (and drop
//     -lgnustep-corebase). Nothing gets renamed.
//
// Declaring the shadowed entry points here also makes *intra-module* calls land on
// mini-CF rather than on corebase, so the module is internally consistent.
//
// Semantics follow Darwin, not corebase: Create*/Copy* return +1, Get* return +0,
// allocator arguments are ignored (NS objects come from the default zone), and
// CFArray/CFDictionary callback arguments are ignored because NS containers always
// retain their elements -- which is exactly kCFType*CallBacks, the only callbacks the
// games pass.

#include "CoreFoundation/CFArray.h"
#include "CoreFoundation/CFBase.h"
#include "CoreFoundation/CFData.h"
#include "CoreFoundation/CFDate.h"
#include "CoreFoundation/CFDictionary.h"
#include "CoreFoundation/CFPropertyList.h"
#include "CoreFoundation/CFString.h"
#include "CoreFoundation/CFUUID.h"
#include "CoreFoundation/CFURL.h"

// drop this (expand to nothing) when gnustep-corebase is unlinked
#define DARWIN_SHADOW(name) __asm__("$darwin$" #name)

// --- allocator / base (CFBase.m) -------------------------------------------------
extern CFAllocatorRef kCFAllocatorDefault DARWIN_SHADOW(kCFAllocatorDefault);
CFTypeRef CFRetain(CFTypeRef cf) DARWIN_SHADOW(CFRetain);
void CFRelease(CFTypeRef cf) DARWIN_SHADOW(CFRelease);
CFTypeRef CFAutorelease(CFTypeRef cf) DARWIN_SHADOW(CFAutorelease);
CFTypeRef CFMakeCollectable(CFTypeRef cf) DARWIN_SHADOW(CFMakeCollectable);

// --- CFString (CFString.m) -------------------------------------------------------
CFStringRef CFStringCreateWithBytes(CFAllocatorRef alloc, const UInt8 *bytes, CFIndex numBytes,
                                    CFStringEncoding encoding, Boolean isExternalRepresentation)
    DARWIN_SHADOW(CFStringCreateWithBytes);
CFStringRef CFStringCreateWithCString(CFAllocatorRef alloc, const char *cStr, CFStringEncoding encoding)
    DARWIN_SHADOW(CFStringCreateWithCString);
CFComparisonResult CFStringCompare(CFStringRef theString1, CFStringRef theString2,
                                   CFStringCompareFlags compareOptions) DARWIN_SHADOW(CFStringCompare);
Boolean CFStringGetCString(CFStringRef theString, char *buffer, CFIndex bufferSize, CFStringEncoding encoding)
    DARWIN_SHADOW(CFStringGetCString);
const char *CFStringGetCStringPtr(CFStringRef theString, CFStringEncoding encoding)
    DARWIN_SHADOW(CFStringGetCStringPtr);

// --- CFArray (CFArray.m) ---------------------------------------------------------
extern const CFArrayCallBacks kCFTypeArrayCallBacks DARWIN_SHADOW(kCFTypeArrayCallBacks);
CFArrayRef CFArrayCreate(CFAllocatorRef allocator, const void **values, CFIndex numValues,
                         const CFArrayCallBacks *callBacks) DARWIN_SHADOW(CFArrayCreate);
CFMutableArrayRef CFArrayCreateMutable(CFAllocatorRef allocator, CFIndex capacity,
                                       const CFArrayCallBacks *callBacks) DARWIN_SHADOW(CFArrayCreateMutable);
void CFArrayAppendValue(CFMutableArrayRef theArray, const void *value) DARWIN_SHADOW(CFArrayAppendValue);
CFIndex CFArrayGetCount(CFArrayRef theArray) DARWIN_SHADOW(CFArrayGetCount);
const void *CFArrayGetValueAtIndex(CFArrayRef theArray, CFIndex idx) DARWIN_SHADOW(CFArrayGetValueAtIndex);

// --- CFDictionary (CFDictionary.m) ------------------------------------------------
CFDictionaryRef CFDictionaryCreate(CFAllocatorRef allocator, const void **keys, const void **values,
                                   CFIndex numValues, const CFDictionaryKeyCallBacks *keyCallBacks,
                                   const CFDictionaryValueCallBacks *valueCallBacks)
    DARWIN_SHADOW(CFDictionaryCreate);
const void *CFDictionaryGetValue(CFDictionaryRef theDict, const void *key) DARWIN_SHADOW(CFDictionaryGetValue);
Boolean CFDictionaryContainsValue(CFDictionaryRef theDict, const void *value)
    DARWIN_SHADOW(CFDictionaryContainsValue);
void CFDictionaryApplyFunction(CFDictionaryRef theDict, CFDictionaryApplierFunction applier, void *context)
    DARWIN_SHADOW(CFDictionaryApplyFunction);

// --- CFData (CFData.m) ------------------------------------------------------------
CFDataRef CFDataCreate(CFAllocatorRef allocator, const UInt8 *bytes, CFIndex length) DARWIN_SHADOW(CFDataCreate);
const UInt8 *CFDataGetBytePtr(CFDataRef theData) DARWIN_SHADOW(CFDataGetBytePtr);
CFIndex CFDataGetLength(CFDataRef theData) DARWIN_SHADOW(CFDataGetLength);

// --- CFBoolean / CFUUID / CFPropertyList (CFMisc.m) --------------------------------
// Do NOT include CoreFoundation/CFNumber.h in this module: it declares the two
// constants below as `const CFBooleanRef`, and mini-CF has to be able to write them at
// startup (see CFMisc.m). The typedef is copied from that header verbatim.
typedef const struct __CFBoolean *CFBooleanRef;
extern CFBooleanRef kCFBooleanTrue DARWIN_SHADOW(kCFBooleanTrue);
extern CFBooleanRef kCFBooleanFalse DARWIN_SHADOW(kCFBooleanFalse);
CFUUIDRef CFUUIDCreate(CFAllocatorRef alloc) DARWIN_SHADOW(CFUUIDCreate);
CFUUIDBytes CFUUIDGetUUIDBytes(CFUUIDRef uuid) DARWIN_SHADOW(CFUUIDGetUUIDBytes);
CFPropertyListRef CFPropertyListCreateFromXMLData(CFAllocatorRef allocator, CFDataRef xmlData,
                                                  CFOptionFlags mutabilityOption, CFStringRef *errorString)
    DARWIN_SHADOW(CFPropertyListCreateFromXMLData);

// --- CFURL (CFURL.m) --------------------------------------------------------------
CFURLRef CFURLCreateWithFileSystemPath(CFAllocatorRef allocator, CFStringRef filePath, CFURLPathStyle pathStyle,
                                       Boolean isDirectory) DARWIN_SHADOW(CFURLCreateWithFileSystemPath);
CFURLRef CFURLCreateFromFileSystemRepresentation(CFAllocatorRef allocator, const UInt8 *buffer, CFIndex bufLen,
                                                 Boolean isDirectory)
    DARWIN_SHADOW(CFURLCreateFromFileSystemRepresentation);
CFURLRef CFURLCreateWithString(CFAllocatorRef allocator, CFStringRef URLString, CFURLRef baseURL)
    DARWIN_SHADOW(CFURLCreateWithString);
Boolean CFURLGetFileSystemRepresentation(CFURLRef url, Boolean resolveAgainstBase, UInt8 *buffer, CFIndex maxBufLen)
    DARWIN_SHADOW(CFURLGetFileSystemRepresentation);
