# mini-CF — demanded CoreFoundation surface and the corebase dependency audit

Scope phase of [REFACTOR.md](../REFACTOR.md) track 2 **option B**: replace
gnustep-corebase's role with a hand-written CF implemented as ObjC calls into
libobjc2/gnustep-base, so toll-free bridging is true by construction rather than by
corebase's two-halves-that-disagree design (the CFURL-vs-CFBundle NULL-deref in
[LOADER.md](../LOADER.md) §2).

Delivery lane: the resolver's `$darwin$` override (`src/lib.cpp` `lib::resolve`, step 1)
wins over corebase's plain exports, so corebase stays linked and every symbol *not* yet
shadowed keeps working. Migration is therefore incremental and per-CF-type.

Sources: `docs/surface-16.3/CoreFoundation.txt`, `docs/surface-16.15/CoreFoundation.txt`,
`build/macholo-dump --syms` on 14.8 (that surface dir only captured libSystem), plus a
`LD_DEBUG=bindings` run of the 16.3 gate replay to GAMELOOP.

---

## 1. Demanded surface

Union of 14.8 + 16.3 + 16.15, leading `_` stripped, ` (weak)` suffixes stripped:
**35 symbols = 25 functions + 4 data/constants + 6 non-CF (objc class refs, one
Foundation global)**. 16.3 and 16.15 are identical except 16.15 adds
`OBJC_CLASS_$_NSURL`; 14.8 additionally imports `CFMakeCollectable` (GC-era no-op,
dropped by the newer SDKs). 25 functions is comfortably inside the "~30 symbols,
hand-rollable" sizing in REFACTOR.md and ~0.8% of CoreFoundation's 3,075 real exports
([MACOS-26.6-SURFACE.md](../MACOS-26.6-SURFACE.md) §3).

Status legend: **corebase** = resolves to gnustep-corebase today, i.e. to-implement;
**shimmed** = already ours in `libstub.so`; **n/a** = not CF work (see §1.7).

### 1.1 Allocator / base

| symbol | kind | 14.8 | 16.3 | 16.15 | today | mini-CF plan |
|---|---|:-:|:-:|:-:|---|---|
| `CFRetain` | fn | y | y | y | corebase | `[obj retain]` |
| `CFRelease` | fn | y | y | y | corebase | `[obj release]` |
| `CFMakeCollectable` | fn | y | — | — | corebase | identity (no GC on Darwin x86_64 either) |
| `kCFAllocatorDefault` | data | y | y | y | corebase | own `NULL` allocator token (CF's documented "default") |

`CFAutorelease`/`CFGetTypeID`/`CFEqual`/`CFHash` are **not** imported by any of the
three binaries; they are implemented anyway only if an internal caller needs them.

### 1.2 CFString

| symbol | kind | 14.8 | 16.3 | 16.15 | today | mini-CF plan |
|---|---|:-:|:-:|:-:|---|---|
| `CFStringCreateWithBytes` | fn | y | y | y | corebase | `-[NSString initWithBytes:length:encoding:]`, **Create ⇒ +1** |
| `CFStringCompare` | fn | y | y | y | corebase | `-compare:options:` (flag translation) |
| `CFStringGetCString` | fn | y | y | y | corebase | `-getCString:maxLength:encoding:`, false if it doesn't fit |
| `CFStringGetCStringPtr` | fn | y | y | y | corebase | needs a *stable* pointer — see §4 |
| `__CFConstantStringClassReference` | data | y | y | y | n/a | guest `__cfstring` literals are rewritten by `src/objc.cpp`; never resolved through the import lane |

### 1.3 CFArray

| symbol | kind | 14.8 | 16.3 | 16.15 | today | mini-CF plan |
|---|---|:-:|:-:|:-:|---|---|
| `CFArrayCreate` | fn | y | y | y | corebase | `+[NSArray arrayWithObjects:count:]`, **+1** |
| `CFArrayCreateMutable` | fn | y | y | y | corebase | `+[NSMutableArray arrayWithCapacity:]`, **+1** |
| `CFArrayAppendValue` | fn | y | y | y | corebase | `-addObject:` |
| `CFArrayGetCount` | fn | y | y | y | corebase | `-count` |
| `CFArrayGetValueAtIndex` | fn | y | y | y | corebase | `-objectAtIndex:`, **+0** |
| `kCFTypeArrayCallBacks` | data | y | y | y | corebase | own `CFArrayCallBacks` whose retain/release are mini-CF's |

### 1.4 CFDictionary

| symbol | kind | 14.8 | 16.3 | 16.15 | today | mini-CF plan |
|---|---|:-:|:-:|:-:|---|---|
| `CFDictionaryCreate` | fn | y | y | y | corebase | `+[NSDictionary dictionaryWithObjects:forKeys:count:]`, **+1** |
| `CFDictionaryGetValue` | fn | y | y | y | corebase | `-objectForKey:`, **+0** |
| `CFDictionaryContainsValue` | fn | y | y | y | corebase | `-[[dict allValues] containsObject:]` (CF compares by the value callbacks' equal, which for `kCFTypeDictionaryValueCallBacks` is `CFEqual`) |
| `CFDictionaryApplyFunction` | fn | y | y | y | corebase | key enumeration + callback |

### 1.5 CFData

| symbol | kind | 14.8 | 16.3 | 16.15 | today | mini-CF plan |
|---|---|:-:|:-:|:-:|---|---|
| `CFDataCreate` | fn | y | y | y | corebase | `+[NSData dataWithBytes:length:]`, **+1** |
| `CFDataGetBytePtr` | fn | y | y | y | corebase | `-bytes`, **+0** |
| `CFDataGetLength` | fn | y | y | y | corebase | `-length` |

### 1.6 CFUUID, CFPropertyList, CFBundle

| symbol | kind | 14.8 | 16.3 | 16.15 | today | mini-CF plan |
|---|---|:-:|:-:|:-:|---|---|
| `CFUUIDCreate` | fn | y | y | y | corebase | `+[NSUUID UUID]`, **+1** |
| `CFUUIDGetUUIDBytes` | fn | y | y | y | corebase | `-getUUIDBytes:` into the 16-byte `CFUUIDBytes` (returned by value) |
| `CFPropertyListCreateFromXMLData` | fn | y | y | y | corebase | `NSPropertyListSerialization`, **+1** |
| `kCFBooleanTrue` | data | y | y | y | corebase | `[NSNumber numberWithBool:YES]` — see §4 |
| `CFBundleGetMainBundle` | fn | y | y | y | corebase | **left on corebase** (see §5) |
| `CFBundleGetIdentifier` | fn | y | y | y | corebase | **left on corebase** (see §5) |
| `CFBundleCopyPreferredLocalizationsFromArray` | fn | y | y | y | shimmed | already ours (`CFBundle.m`, returns NULL = "no preference") |

### 1.7 Imported from CoreFoundation but not CF work

`OBJC_CLASS_$_NSArray`, `OBJC_CLASS_$_NSDate`, `OBJC_CLASS_$_NSMutableArray`,
`OBJC_CLASS_$_NSMutableDictionary`, `OBJC_CLASS_$_NSURL` — class references, killed by
the resolver's `BLOCKED` prefix list and satisfied by `src/objc.cpp`'s class lookup.
`NSDefaultRunLoopMode` — a gnustep-**base** global, already resolved there.
`__CFConstantStringClassReference` — see §1.2.

### 1.8 CF surface already shimmed in `libstub.so`

`nm -D --defined-only build/stub/libstub.so`, 17 CF-ish exports, none of them in the
demanded set except the first:

- `CFBundleCopyPreferredLocalizationsFromArray`, `CFBundleGetAllBundles`,
  `CFBundleCopyInfoDictionaryForURL`, `CFBundleCopySharedSupportURL`,
  `CFBundleGetVersionNumber` — gaps corebase doesn't implement (WC3-era).
- `$darwin$CFURLCreateWithFileSystemPath`, `$darwin$CFURLCreateFromFileSystemRepresentation`,
  `$darwin$CFURLCreateWithString`, `$darwin$CFURLGetFileSystemRepresentation` —
  the existing NSURL-backed CFURL, i.e. mini-CF's prototype. **No CFURL symbol is
  imported by any League build**; this exists for WC3 and is the pattern to extend.
- `CFURLCreateFromFSRef` (Carbon), `CFNotificationCenter{GetLocalCenter,
  GetDistributedCenter,AddObserver,RemoveObserver}`, `CGDataProviderCreateWithCFData`,
  `IORegistryEntryCreateCFProperty`, `IOHIDSetCFTypeParameter` — other frameworks'
  CF-typed entry points, out of scope.

---

## 2. Does gnustep-base need corebase? — **No**

REFACTOR.md's open question ("does dropping gnustep-corebase strand any gnustep-base
internals that call into it?"), answered by `readelf -d` over every `.so` in
`deps/sysroot`:

| library | `DT_NEEDED` |
|---|---|
| `libgnustep-base.so.1.29` | libobjc, libgmp, libgnutls, libxslt, libxml2, libffi, libz, libicui18n, libdispatch, libcurl, libm, libgcc_s, libc — **no corebase** |
| `libgnustep-corebase.so.0` | libm, **libobjc**, libicui18n, **libgnustep-base.so.1.29**, libc |
| `libobjc.so.4.6` | libc++abi, libm, libc++, libgcc_s, libc |

The dependency is one-directional: **corebase → base**, never base → corebase. Their
exported symbol sets are disjoint (0 names in common). Nothing in gnustep-base is
stranded by removing corebase; the blockers are all on our side (§5).

## 3. What the running process actually binds from corebase — 29 symbols

`LD_DEBUG=bindings` over a full 16.3 gate run (reaches `GAMESTATE_GAMELOOP`), filtered
to bindings *resolved into* `libgnustep-corebase.so.0`. 270 unique symbols bind there,
but 250 are corebase resolving its own internals (its `NSCF*` class refs and ivar
offsets). Only 20 + 9 come from outside:

**From `build/macholo` (i.e. the guest's imports through `lib::resolve` + the loader's
own `dlsym`) — 29:**

- the 23 demanded CF functions of §1 minus the two already-shimmed ones:
  `CFArray{AppendValue,Create,CreateMutable,GetCount,GetValueAtIndex}`,
  `CFBundle{GetIdentifier,GetMainBundle}`, `CFData{Create,GetBytePtr,GetLength}`,
  `CFDictionary{ApplyFunction,ContainsValue,Create,GetValue}`,
  `CFPropertyListCreateFromXMLData`, `CFRelease`, `CFRetain`, `CFStringCompare`,
  `CFStringCreateWithBytes`, `CFStringGetCString`, `CFStringGetCStringPtr`,
  `CFUUIDCreate`, `CFUUIDGetUUIDBytes`
- the 3 data constants: `kCFAllocatorDefault`, `kCFBooleanTrue`, `kCFTypeArrayCallBacks`
- **3 loader-internal ones, from `src/objc.cpp`'s `__cfstring` converter**:
  `CFStringGetTypeID`, `_CFRuntimeGetClassWithTypeID`, `._OBJC_CLASS_NSCFString`

**From `libstub.so` — 3:** `CFRetain`, `CFRelease`, `kCFTypeArrayCallBacks` (our own
`CFBundle.m` and friends calling plain CF names).

Everything else corebase exports (912 dynamic symbols) is dead weight in this process.

### 3.1 The uncomfortable part: guest CF strings are corebase objects

`src/objc.cpp` rewrites each guest `__cfstring` literal into a gnustep-shaped CFString
whose `isa` is corebase's `NSCFString` and whose `typeID` comes from corebase's
`CFStringGetTypeID()`, and patches corebase's `CFRuntime` class table to install a
Darwin-compatible hash. corebase provides the whole toll-free bridge class family
(`NSCFString`, `NSCFArray`, `NSCFDictionary`, `NSCFData`, `NSCFNumber`, `NSCFType`, …);
gnustep-base provides **none** of them.

Two consequences:

1. **Incremental shadowing is safe in the direction we need.** A CF object produced by
   corebase *is* an ObjC object of an `NSString`/`NSArray`/`NSDictionary` subclass, so a
   mini-CF function that `objc_msgSend`s it works whether the object came from corebase
   or from mini-CF. Guest string literals included.
2. **The converter is the real blocker to deleting corebase**, not the CF API surface —
   and it lives in `src/`, off-limits for this track. See §5.

---

## 4. Semantics decided up front

- **Get vs Copy/Create.** `Create*`/`Copy*` return +1 (the caller releases);
  `Get*` return +0. Every mini-CF `Create` therefore `-retain`s an autoreleased NS
  object; every `Get` returns the object without retaining.
- **`CFStringGetCStringPtr`.** Darwin returns a pointer into the string's own storage
  or NULL, and callers are required to fall back to `CFStringGetCString`. gnustep's
  `-UTF8String` hands back a *fresh autoreleased* buffer on every call (verified:
  `GSTinyString` returns a different pointer each time), which is neither stable nor
  ownership-compatible. mini-CF therefore memoises: a process-wide
  `NSString → NSData(UTF-8 + NUL)` cache keyed by string value, returning the cached
  bytes. Stable pointer, no dangling, bounded by the number of distinct strings queried.
- **`kCFBooleanTrue`.** Verified empirically that gnustep-base's
  `+[NSNumber numberWithBool:YES]` is a singleton (`NSBoolNumber`) **and** that
  `NSPropertyListSerialization` returns that same singleton for `<true/>`, so the
  pointer-identity comparison games do (`value == kCFBooleanTrue`) keeps working once
  both the constant and the plist parser are mini-CF's. Both must move in the same
  commit or the identity breaks.
- **Allocators are ignored.** Every `CFAllocatorRef` parameter is dropped; NS objects
  come from the default zone. `kCFAllocatorDefault` becomes a NULL token.
- **Callbacks are ignored.** `CFArrayCallBacks`/`CFDictionary*CallBacks` arguments are
  dropped: NS containers always retain/release their elements, which is exactly
  `kCFType*CallBacks` — the only callbacks any of the three binaries pass.
- Anything whose Darwin behaviour can't be reproduced honestly gets a `panic()` stub
  rather than a plausible wrong answer.

---

## 5. What landed

`stub/src/corefoundation/`, six commits, one per CF type group; `minicf.h` holds the
design note and the `DARWIN_SHADOW` asm-label mechanism. 25 new `$darwin$` exports
(baseline 662 → 685):

| group | file | entry points |
|---|---|---|
| base | `CFBase.m` | `CFRetain`, `CFRelease`, `CFAutorelease`, `CFMakeCollectable`, `kCFAllocatorDefault` |
| CFString | `CFString.m` | `CFStringCreateWithBytes`, `CFStringCreateWithCString`, `CFStringCompare`, `CFStringGetCString`, `CFStringGetCStringPtr` |
| CFArray | `CFArray.m` | `CFArrayCreate`, `CFArrayCreateMutable`, `CFArrayAppendValue`, `CFArrayGetCount`, `CFArrayGetValueAtIndex`, `kCFTypeArrayCallBacks` |
| CFDictionary | `CFDictionary.m` | `CFDictionaryCreate`, `CFDictionaryGetValue`, `CFDictionaryContainsValue`, `CFDictionaryApplyFunction` |
| CFData | `CFData.m` | `CFDataCreate`, `CFDataGetBytePtr`, `CFDataGetLength` |
| misc | `CFMisc.m` | `CFUUIDCreate`, `CFUUIDGetUUIDBytes`, `CFPropertyListCreateFromXMLData`, `kCFBooleanTrue`, `kCFBooleanFalse` |
| CFURL | `CFURL.m` | pre-existing, converted from hand-written `$darwin$` names to the macro |

Verified the same way §3 was measured: after the six groups, a 16.3 run to GAMELOOP
binds **25 `$darwin$CF*` symbols out of `libstub.so`** and only **5** symbols out of
corebase — `CFBundleGetMainBundle`, `CFBundleGetIdentifier` (guest) and
`CFStringGetTypeID`, `_CFRuntimeGetClassWithTypeID`, `._OBJC_CLASS_NSCFString`
(`src/objc.cpp`). Gates: 16.3 PASS after every group, 14.8 PASS at phase end; the 16.3
log is identical to the pre-mini-CF baseline modulo thread ids and pointer values.

Deliberately **not** shadowed: `CFGetTypeID`/`CFEqual`/`CFHash` (no binary imports
them, and a CF type ID for an NS object would have to be invented), and all of
`CFBundle` beyond the gap-fillers already in `CFBundle.m`.

## 6. Residual distance to actually deleting corebase — measured

Experiment (built, run, then reverted; nothing below is landed): drop
`-lgnustep-corebase` from `stub/gnustep/CMakeLists.txt`, which is the single place it
enters the link — the `GNUStep` INTERFACE target every module and the loader use.

**Link stops with 8 undefined symbols, all referenced by `libstub.so` itself:**

| symbol | referenced by | distance |
|---|---|---|
| `CFRetain`, `CFRelease` | `coregraphics/CG.m` | one line: mini-CF already implements them, `CG.m` just calls the plain names |
| `CFStringCreateWithCString`, `CFArrayCreate`, `kCFTypeArrayCallBacks` | `carbon/stub_carbon.m` | same — mini-CF has all three |
| `CFBundleCreate`, `CFBundleCopyBundleURL`, `CFBundleGetInfoDictionary` | `corefoundation/CFBundle.m` | **real work**: needs an NSBundle-backed CFBundle |

Rerouting the first five to mini-CF (asm-labelled externs, the same trick the module
uses internally) and stubbing the three CFBundle calls makes the link **completely
clean** — 0 undefined symbols, no corebase in `DT_NEEDED`. The process then gets
further than expected and dies at:

```
unresolved at ... _CFBundleGetIdentifier +0
unresolved at ... _CFBundleGetMainBundle +0
src/objc.cpp:401 _CFRuntimeGetClassWithTypeID != nullptr
```

(the eight `_OBJC_CLASS_$_MTL*`/`CAMetalLayer` "unresolved objc" lines above it are
pre-existing — Metal is not implemented — and appear in a normal green run too.)

So the residual distance is exactly three things, in increasing size:

1. **CFBundle** — `CFBundleGetMainBundle`, `CFBundleGetIdentifier` (guest imports) plus
   `CFBundleCreate`, `CFBundleCopyBundleURL`, `CFBundleGetInfoDictionary` (our own
   callers). corebase's CFBundle is itself a thin shim over gnustep-base `NSBundle`, so
   this is a mechanical port, but `CFBundle.m` currently and deliberately does *not*
   shadow the getters (the game feeds the `CFBundleRef` back into corebase), so it has
   to move as one piece.
2. **Two cross-module callers** (`CG.m`, `stub_carbon.m`) that call plain CF names and
   need to see mini-CF's declarations — one include each, once `minicf.h` (or a public
   header split out of it) is reachable from other modules.
3. **`src/objc.cpp`'s `__cfstring` converter** — the hard one, and the only one outside
   `stub/`: it needs a `CFStringGetTypeID`, a `_CFRuntimeGetClassWithTypeID` whose
   returned class table it can patch, and an `NSCFString` class to use as the literals'
   `isa`. Under mini-CF the natural answer is to stop pretending literals are CFRuntime
   objects and give them a gnustep constant-string `isa` directly, but that is a `src/`
   change and out of scope for this track.
