# macOS 26.6 export surface vs. League 16.3/16.15 import demand

Cursory cross-reference of a macOS 26.6 (25G72, arm64, Darwin 25.6.0) system-library
export dump against the game's known import surface, done to sanity-check macholo's
stub coverage and to size the `docs/REFACTOR.md` track-2B mini-CF work. Dump path:
`/home/user/projects/osx-libs/macos-exports-26.6-arm64/` (137 files, manifest in
`MANIFEST.txt`; per-library `<name>.<arch>.txt` from `dyld_info -exports`, plus
`<name>.objc.txt` from `dyld_info -objc`). Demand side: `docs/surface-16.3/` and
`docs/surface-16.15/` in this worktree (see their READMEs for format).

**Headline: coverage is excellent (~97-100% per framework) but the handful of misses
are real and informative** — two are outright Apple API removals since the game's SDK
(`Gestalt`, `NSMarkedClauseSegmentAttributeName`), one is a removal that affects both
architectures equally (`objc_msgSend_stret`, gone from x86_64 *and* arm64 — so even
real current macOS can't launch this binary without a shim), and the big one is that
**classic desktop OpenGL is functionally gone from `OpenGL.framework`**: 0 of ~99 `gl*`
draw-call symbols remain on either arch, and even `CGLGetCurrentContext`/
`CGLSetCurrentContext` are absent — only context lifecycle (create/destroy/retain)
survives. This doesn't hurt macholo (the plan already routes `gl*` to host libGL,
bypassing Apple's framework entirely) but it does mean 16.3's GL path can no longer run
on unmodified current macOS at all. Separately, `dyld_info -objc` came back **empty for
every single framework sampled** (2-line header only, even for AppKit/Foundation) —
this is a tooling/on-disk-stub limitation, not reality, so no ObjC class-list data was
recoverable from this dump; a class-dump-style tool against the extracted shared cache
would be needed instead.

## 1. Coverage sanity (surface-16.3, x86_64 slice vs. `*.x86_64.txt`)

Comparison normalizes one leading underscore, strips ` (weak)` markers, and treats
`[re-export]` lines (common in CoreGraphics/AppKit/Foundation, which re-export from
private frameworks like SkyLight) as present. `AudioUnit`, `SystemConfiguration`,
`RPatch`, `mvg` have no matching dump file (see §5) and are excluded below.

| Framework | Imported | Missing | Coverage | Missing symbols |
|---|---:|---:|---:|---|
| AppKit | 28 | 1 | 96.4% | `NSMarkedClauseSegmentAttributeName` |
| AudioToolbox | 11 | 0 | 100% | — |
| CoreAudio | 3 | 0 | 100% | — |
| CoreFoundation | 33 | 0 | 100% | — |
| CoreGraphics | 21 | 0 | 100% | — |
| CoreServices | 1 | 1 | 0% | `Gestalt` |
| CoreVideo | 2 | 0 | 100% | — |
| Foundation | 12 | 0 | 100% | — |
| IOKit | 5 | 0 | 100% | — |
| Metal | 8 | 0 | 100% | — |
| **OpenGL** | 111 | 110 | **0.9%** | all `gl*` (99) + `CGLGetCurrentContext` (see below) |
| QuartzCore | 1 | 0 | 100% | — |
| Security | 10 | 0 | 100% | — |
| libobjc | 22 | 1 | 95.5% | `objc_msgSend_stret` |
| libresolv | 2 | 0 | 100% | — |
| libSystem | 389 | 6 | 98.5% | `fstat$INODE64`, `lstat$INODE64`, `stat$INODE64`, `opendir$INODE64`, `readdir$INODE64`, `select$1050` |

Notes on each real miss:

- **`Gestalt`** — fully absent from `CoreServices`/`Carbon`/anywhere in the dump (both
  arches). Long-deprecated (Mac OS X 10.8+), apparently now actually removed from the
  export trie rather than just discouraged.
- **`NSMarkedClauseSegmentAttributeName`** — the plain `NSAttributedStringKey` global
  is gone; AppKit now only exposes the concept via a Swift-mangled
  `AttributeScopes.AppKitAttributes.MarkedClauseSegmentAttribute` symbol family. A
  genuine API-shape migration, not just a rename.
- **`objc_msgSend_stret`** — see §2, gone from both arches, not an arm64-only thing.
- **libSystem's 6 misses** are all retired *symbol-versioning suffixes*
  (`$INODE64`, `$1050`, `$DARWIN_EXTSN`, `$NOCANCEL` are still used for a few other
  calls, e.g. `_realpath$DARWIN_EXTSN`, `_close$NOCANCEL` survive) — the plain
  unsuffixed name (`_stat`, `_fstat`, `_lstat`, `_opendir`, `_readdir`, `_select`) is
  present and is now the sole implementation. Functionally 100% covered.
- **libSystem union** was built from `libSystem.B` + all 22 `libsystem_*` dylibs +
  `libdyld`/`libdispatch`/`libxpc`/`libcompiler_rt`/`libcache`/`libcommonCrypto`/
  `libcopyfile`/`libcorecrypto`/`libkeymgr`/`libmacho`/`libquarantine`/`libremovefile`/
  `libunwind` — the umbrella `libSystem.B.dylib` itself only directly exports 3 symbols
  (`___crashreporter_info__`, `_libSystem_init_after_boot_tasks_4launchd`,
  `_mach_init_routine`); everything else routes through a sub-dylib.
- **OpenGL**: the dump's `OpenGL.x86_64.txt`/`.arm64.txt` (83 lines) contains **only**
  `CGL*`/`GLC*`/`glc*` internal-plumbing symbols — zero `gl*` GL1/2/3-style draw calls
  on either architecture, and critically `CGLGetCurrentContext`/`CGLSetCurrentContext`
  aren't there either (only create/destroy/copy/retain/release/lock context lifecycle
  remains). 16.15's surface already dropped OpenGL entirely (Metal-only); 16.3 still
  hard-imports it non-weak. See §5.
- 16.15's `libSystem`/`libobjc` demand lists are supersets/near-identical to 16.3's;
  same 6 + 1 misses apply there too.

## 2. libobjc: `objc_msgSend*` family, arm64 vs. x86_64

```
ARM64:                          X86_64:
_objc_msgSend                   _objc_msgSend
_objc_msgSendSuper              _objc_msgSendSuper
_objc_msgSendSuper2             _objc_msgSendSuper2
_objc_msgSendSuper2_debug        _objc_msgSendSuper2_debug
_objc_msgSend_debug              _objc_msgSend_debug
_objc_msgSend_noarg               _objc_msgSend_noarg
```

**Delta: none.** The sets are byte-for-byte identical on both arches in this build.
The historically arm64-only-missing `stret`/`fpret`/`fp2ret` family
(`objc_msgSend_stret`, `objc_msgSendSuper_stret`, `objc_msgSendSuper2_stret`,
`objc_msgSend_fpret`, `objc_msgSend_fpret2`) is **entirely absent from x86_64 too** on
26.6 — not just an arm64 ABI quirk as originally hypothesized, it's been dropped
system-wide. Since both 16.3 and 16.15 strongly (non-weak) import
`_objc_msgSend_stret`, this means the game literally cannot dyld-load on an unmodified
current-macOS x86_64 machine either — macholo's own `objc_msgSend_stret` shim isn't
compensating for an arm64 port gap, it's compensating for something Apple has already
removed upstream. **Sizing implication for a future `arch-aarch64` module**: no
separate stret/fpret dispatch thunks are needed for parity with 26.6's real ABI surface
(there's nothing to be parity with) — only the plain `objc_msgSend`/`msgSendSuper`/
`msgSendSuper2` shapes, same as x86_64. The one AArch64-specific concern that remains
out of scope per `docs/REFACTOR.md`'s explicit gap list is calling-convention codegen
for the actual thunks, not variant *selection*.

## 3. Mini-CF sizing (REFACTOR.md track 2B)

- **Total CoreFoundation arm64 exports: 3,075** (identical count on x86_64; export
  *names* match 1:1 across arches, no re-exports in this file).
- **Game-imported CF symbols: 34** (union of surface-16.3's 33 + surface-16.15's 34 —
  16.15 adds `_OBJC_CLASS_$_NSURL`, everything else identical). That's ~1.1% of the
  full CF export surface — consistent with "mini-CF" being a small, hand-rollable
  shim rather than a real CF reimplementation.
- **CoreFoundation.objc.txt class list: 0 classes** (file is a 2-line header only —
  `-objc` returned nothing). This is true of **every** `.objc.txt` file sampled (18/18
  frameworks, including AppKit and Foundation, which certainly do define ObjC classes)
  — a dump/tooling gap, not reality (see §5). **No `__NSCF*` toll-free-bridge class
  list is recoverable from this dump.** The game's own `objc-classes.txt` demand lists
  (39 classes across both versions) contain no `__NSCF*`-prefixed names either, which
  is expected — apps reference the public class names (`NSArray`, `NSString`, ...),
  not the private CF-bridged implementation classes. If track 2B needs the real
  `__NSCFString`/`__NSCFArray`/`__NSCFDictionary`/etc. list, it'll have to come from
  `class-dump`/`otool` against an extracted shared-cache image, not this dump.

## 4. libsystem sublibrary -> `stub/src/libsystem/` mapping

Rough export counts (arm64) and closest macholo file(s). Apple splits `libsystem_*.dylib`
by fairly fine-grained concern; macholo currently groups by broader functional domain,
so most mappings are many-to-one or reach across Apple boundaries:

| Apple library (exports) | Closest macholo file(s) | Note |
|---|---|---|
| `libsystem_kernel` (1,584) | `stub_kernel.c`, `stub_mach_vm.c`, `stub_kernel_kqueue.c`, + syscall parts of `stub_fs.c`/`stub_process.c`/`stub_socket.c`/`stub_io.c` | **Biggest boundary divergence**: Apple puts *every* BSD/mach syscall (file, socket, process, mach port/task/thread) in one dylib; macholo splits by domain across 6 files |
| `libsystem_c` (1,298) | `stub_str.c`, `stub_rune.c`, + libc-wrapper parts of `stub_fs.c` (`opendir`/`readdir`/`fopen`), `stub_process.c` (`fork`), `stub_sys.c` (`sysctlbyname`, `uuid_*`) | Most POSIX-standard names (string/stdio/math) are satisfied for free by linking host glibc, not by a macholo shim at all |
| `libsystem_m` (444) | `stub_math.c` (2 functions: `__exp10`/`__exp10f`) | Everything else free via host `-lm` (GNUStep link line) — only Darwin-only names need shims |
| `libsystem_pthread` (209) | `stub_pthread.c` | Clean 1:1 domain match; also carries libc++ `std::mutex`/`std::thread`/`std::condition_variable` ABI shims (cross-cutting into libc++abi territory) |
| `libsystem_malloc` (118) | `stub_malloc.c` (1 function: `malloc_size`) | `malloc`/`free`/`calloc`/`realloc` pass straight through to host allocator under the same names — no shim needed |
| `libsystem_asl` (219) | `stub_asl.c` (6 functions, all "don't care" no-ops) | Intentional: logging isn't load-bearing |
| `libdispatch` (408) | `stub_dispatch.c` (1 function, and it's a `panic()` stub) | **Real gap**: game demands 5 `dispatch_*` symbols (`dispatch_data_create`, `dispatch_once_f`, `dispatch_semaphore_{create,signal,wait}`); only the first exists and it panics. The other 4 aren't implemented anywhere in the worktree. |
| `libxpc` (800) | `stub_xpc.c` (3 functions, all `panic()`) | Game only needs `bootstrap_look_up`/`bootstrap_port`/`bootstrap_strerror`; `bootstrap_port` itself is a data symbol in `stub_kernel.c`, not `stub_xpc.c` |
| `libsystem_info` (431) | Partially `stub_socket.c` (`getaddrinfo`/`getifaddrs`/`getnameinfo`/`if_nametoindex`), `stub_compat.c` (`getpwuid`) | No dedicated file; split across two others by call shape rather than by Apple's library boundary |
| `libsystem_platform` (179) | `stub_compat.c` (`OSAtomicCompareAndSwap32/64Barrier`, `OSMemoryBarrier`) | Not currently game-demanded (no `OSAtomic*`/`os_unfair_lock` import found in either surface) |
| `libdyld` (218) | `stub_compat.c` (legacy `NSCreateObjectFileImageFromMemory`/`NSLinkModule`/`NSAddressOfSymbol`/... `NSModule` API, `_tlv_atexit`) | `stub_compat.c` is explicitly a cross-library grab-bag (spans libdyld + libsystem_platform + libsystem_info + Carbon `DebugStr`) |
| `libresolv.9` (155) | `stub_resolv.c` (2 functions: `res_9_query`, `res_9_dn_skipname`, both `panic()`) | Exactly matches the 2 symbols the game actually imports — fully "covered" by demand even though both are stubs that abort |
| `libsystem_blocks` (19) | *(none found)* | Game imports `__Block_object_assign`/`__Block_object_dispose`; no macholo source defines them — presumably expected to come from a Blocks runtime pulled in via `-fblocks` compilation, but that dependency isn't obvious from the CMake files. Worth double-checking this doesn't silently `dlsym` to nothing. |
| `libsystem_containermanager` (423), `libsystem_sandbox` (167), `libsystem_networkextension` (188), `libsystem_dnssd` (61), `libsystem_configuration` (36), `libsystem_notify` (23), `libsystem_coreservices` (18), `libsystem_symptoms` (10), `libsystem_secinit` (5), `libsystem_featureflags` (2), `libsystem_darwin` (75), `libsystem_collections` (57) | *(none)* | Not game-demanded (sandbox/container/network-extension/DNS-SD APIs); no macholo stub needed today |
| `libcorecrypto` (1,092), `libcommonCrypto` (245), `libcache` (34), `libcompiler_rt` (73), `libcopyfile` (11, partial via `stub_fs.c`'s `copyfile*`), `libkeymgr` (11), `libmacho` (90), `libquarantine` (78), `libremovefile` (14), `libunwind` (43), `libiconv.2` (95), `libicucore.A` (8,838) | mostly none, except `copyfile*` in `stub_fs.c` | Private/internal-use libs the game doesn't call directly (reached only via Security/CF, which macholo reimplements at a higher level) |
| `libc++abi` (`__cxa_throw` etc.) | `stub_sys.c` (`__cxa_throw`) | `stub_sys.c` also reaches outside `libsystem` entirely — it shims a C++ ABI symbol, not a Darwin syscall |

Overall pattern: macholo's file boundaries track *implementation shape* (BSD syscall
vs. libc wrapper vs. "we don't care, no-op") more than Apple's *library* boundaries,
which mostly track *build/versioning* concerns. The two are frequently orthogonal,
which is why almost every macholo file pulls from 2+ Apple dylibs and several Apple
dylibs are covered by zero or by fragments of several macholo files.

## 5. Surprises

- **`dyld_info -objc` returns nothing for every framework sampled** (18/18 `.objc.txt`
  files are a 2-line header only) — including AppKit, Foundation, CoreFoundation,
  Metal, QuartzCore, which certainly define/register ObjC classes. Likely because
  these on-disk framework paths on 26.6 are dyld-shared-cache stub files (real
  segments/metadata live only in the merged cache); `dyld_info -objc` apparently can't
  (or wasn't run in a way that could) pull classlist data back out of the cache for
  them, even though `-exports` works fine via the trie. Any future objc-class-surface
  work needs a different extraction method.
- **Classic OpenGL is functionally dead on 26.6**: `OpenGL.framework` still exists and
  loads, but exports zero `gl*` draw-call symbols on either architecture, and is
  missing `CGLGetCurrentContext`/`CGLSetCurrentContext` too — only context
  lifecycle management remains. 16.3 (non-weak) imports the full set; **this game's
  legacy GL renderer path can no longer run on any unmodified current Mac**, which
  actually strengthens the case for macholo's plan to just map `gl*` straight to host
  libGL rather than trying to satisfy them via any real Apple framework.
- **`objc_msgSend_stret` is gone from x86_64 too**, not just arm64 (§2) — same
  real-macOS-can't-launch-this-binary situation as OpenGL, for a different symbol.
- **Two frameworks in the demand list have no corresponding dump file at all**:
  `AudioUnit.framework` and `SystemConfiguration.framework` weren't captured in this
  dump collection, so their 4 and 1 imports respectively couldn't be coverage-checked
  here (not necessarily missing on the real system — just not dumped).
  `RPatch` (12 imports) and `mvg` (6 imports) are Riot-internal libraries bundled with
  the game (patcher / anti-cheat-adjacent), not system frameworks, so they were never
  expected to have a dump entry.
- **`ApplicationServices.framework` and `Carbon.framework` export almost nothing**
  (5-line files, just a version-number/string pair each) — both are umbrella
  frameworks on modern macOS that just re-export their sub-frameworks; direct symbol
  demand against them (like `CoreServices`'s lone `Gestalt` import) can't be satisfied
  by the umbrella itself.
- **Symbol-versioning suffixes are being retired**: `$INODE64`/`$1050`-suffixed names
  the game imports (`stat$INODE64`, `select$1050`, etc.) no longer exist as separate
  exports — only the unsuffixed name remains, now the sole/default implementation.
  Functionally harmless (macholo already implements these under the suffixed name,
  which will keep working since it's the *binary's* import name being satisfied, not
  the host's), but worth knowing if anyone diffs against a fresh dump later expecting
  the suffix to still be there.
- All system libraries in this dump self-report as `arm64e` slices (see e.g.
  `AppKit.framework/AppKit [arm64e]:` header) even in the files named `*.arm64.txt` —
  modern macOS ships no plain-arm64 slice, only arm64e; worth remembering when
  eyeballing raw dump headers.
- `libicucore.A` is enormous (8,838 exports) relative to everything else in the
  dump — by far the largest single library, dwarfing even `libsystem_kernel`.
