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

#include "CoreFoundation/CFBase.h"
#include "CoreFoundation/CFURL.h"

// drop this (expand to nothing) when gnustep-corebase is unlinked
#define DARWIN_SHADOW(name) __asm__("$darwin$" #name)

// --- allocator / base (CFBase.m) -------------------------------------------------
extern CFAllocatorRef kCFAllocatorDefault DARWIN_SHADOW(kCFAllocatorDefault);
CFTypeRef CFRetain(CFTypeRef cf) DARWIN_SHADOW(CFRetain);
void CFRelease(CFTypeRef cf) DARWIN_SHADOW(CFRelease);
CFTypeRef CFAutorelease(CFTypeRef cf) DARWIN_SHADOW(CFAutorelease);
CFTypeRef CFMakeCollectable(CFTypeRef cf) DARWIN_SHADOW(CFMakeCollectable);

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
