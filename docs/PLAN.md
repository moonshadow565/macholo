# macholo — roadmap (2026-07)

Decision: **evolve macholo, do not restart.** The parser/loader/objc-converter/TLV core is
sound and proven; the pain is the implicit dependency swamp (system-installed GNUstep tree,
prebuilt tarball, apt-based build script) and a few loader shortcuts. Those get replaced;
the code doesn't.

Reference target: League of Legends 16.15 x86_64 slice (`Game/16-5`), analyzed in
[AUDIT-16.15.md](AUDIT-16.15.md). Exact import surfaces live in
[surface-16.15/](surface-16.15/).

## Prior art

| | macholo | touchHLE | Darling |
|---|---|---|---|
| Philosophy | HLE at framework boundary, native exec | same — own impls of Foundation/UIKit/etc | port Apple's whole stack (dyld, objc4, CF, AppKit) |
| CPU | native x86_64 | dynarmic ARM emu (32-bit iOS 2.x–3.x) | native |
| ObjC | GNUstep libobjc2 + metadata converter | own Rust runtime, consumes Apple ABI directly | Apple objc4 port |
| CF/Foundation | GNUstep base + corebase | own, exactly the subset apps use | Apple CF port |
| Outcome | League replays ran (GL era) | real games work, per-game incremental support | everything half-works, undebuggable hangs |

Lesson (validated by touchHLE, and by our Darling experience): implement exactly what the
target apps use, own every layer you implement, grow support per-game. macholo is already
on the right side of this line. Games genuinely don't need much: League 16.15's entire
non-libc/libc++ surface is ~135 framework symbols + 29 objc classes.

## Dependency decisions

- **ObjC runtime: keep GNUstep libobjc2, vendor it** (submodule/FetchContent +
  `add_subdirectory`; it's clean CMake with ~no deps). Writing an own Apple-ABI runtime
  touchHLE-style is feasible (~20 entry points + dispatch + class tables) and would delete
  the converter, but re-solves exceptions interop, ARC semantics, root-class behavior for
  little gain. Revisit only if libobjc2 itself becomes the pain.
- **CoreFoundation: write mini-CF (~30 symbols), drop corebase/Hurd-CF.** Implement CF
  types *as* libobjc2 objects so CFRetain/CFRelease and toll-free bridging with NSString/
  NSArray are trivially coherent. Apple's real CF
  (`swiftlang/swift-corelibs-foundation/Sources/CoreFoundation`, plain C + CMakeLists,
  builds on Linux with clang) is the reference/parts bin, not a dependency.
- **Foundation: two-stage.** Stage 1: keep gnustep-base to get back to a running state,
  but build it reproducibly (podman, below). Stage 2: replace with a mini-Foundation for
  the ~15 NS classes actually used (same approach as the hand-written AppKit stubs).
  Instrument dispatch during stage 1 to size stage 2 with data. Drop libs-gui / libs-back /
  corebase / tools-make now — nothing imports them; the AppKit stubs + SDL2 are the real
  backend.
- **Reproducible dep builds: podman.** One Containerfile building libobjc2 (+ temporarily
  gnustep-base) with clang/libc++ into a sysroot the CMake build consumes. Replaces
  `build-gnustep.sh` (apt-based) and the mystery `GNUStep.tar.xz`.

## Phases

### Phase 0 — hygiene + tooling (small, high leverage)
- [ ] FAT header slicing (port from customlolskins-manager); select slice by host arch.
- [ ] Section search across **all** segments — `__DATA_CONST` now holds `__mod_init_func`,
      `__objc_classlist`, `__objc_protolist`, `__cfstring`; `find_data_section()` only
      searches `__DATA`, so initializers + objc registration silently no-op on new binaries.
- [ ] Audit tool: `macholo --audit <binary>` → import surface (per-dylib symbols, objc
      classes/sels) diffed against implemented surface. Turns "will game X run?" into a
      printed to-do list. Parsing already exists; `docs/surface-16.15/` is the manual
      prototype of its output.
- [ ] Podman dep build; `cmake -B build && cmake --build build` works on Fedora 44.

### Phase 1 — League 16.15 reaches main(), runs replay headless
- [ ] Multi-image loader: load `@rpath` frameworks (RPatch, mvg, RiotGamesApi + its two
      dylibs), per-image rebase/bind/objc/TLV, cross-image export resolution. Biggest
      loader gap (macholo is single-image today; 18 lazy binds land in RPatch/mvg).
- [ ] Modern objc entry-point shims: `objc_alloc`, `objc_alloc_init`, `objc_opt_class`,
      `objc_opt_isKindOfClass`, `objc_opt_new`, `objc_opt_self`, `objc_msgSendSuper2`
      (non-fixup). The old `*_fixup` variants in `stub_objc.S` are no longer referenced.
- [ ] Mini-CF (~30 syms, see `surface-16.15/CoreFoundation.txt`) incl.
      `__CFConstantStringClassReference` for `__cfstring` literals and
      `CFPropertyListCreateFromXMLData`.
- [ ] Resolve Metal imports to NULL (Metal/CoreGraphics are weak dylibs; MTL* class
      imports are weak) → game should fail renderer init gracefully.
- Milestone: replay loads, sim runs, "Failed to init renderer" — old milestone reproduced
  on the modern client.

### Phase 2 — renderer (the real new work)
16.15 is **Metal-only**: zero OpenGL.framework binds, no NSOpenGL*/Cg imports (the old
NSOpenGLContext + stub_nvcg path served the GL-era client). Options, in order:
- [ ] Spike: what does the game feed Metal — runtime MSL source
      (`newLibraryWithSource:`) or precompiled `.metallib` (AIR bitcode)? This decides
      feasibility: MSL source → constrained translator over Riot's shader corpus is
      tractable; metallib → LLVM AIR parsing, much harder.
- [ ] Mini-Metal over Vulkan (or GL 4.6) as another stub framework, sized by the audit
      tool against actual runtime calls (device/queue/encoder protocols arrive via
      msgSend, invisible in static imports — instrument dispatch to enumerate).
      Prior art to study, not adopt: Darling's `indium`. Months, not weeks — but it's the
      differentiating component, and exactly touchHLE's playbook applied to Metal.
- [ ] Pragmatic parallel track: keep a GL-era League build as the "renders on screen"
      demo while Metal HLE matures.

### Phase 3 — mini-Foundation, drop gnustep-base
Sized by phase-1 selector logs. Expect ~15 classes, shallow usage (paths, bundle, process
info, strings).

### Phase 4 — more games
Pick 2–3 macOS-only x86_64 titles; run the audit tool; implement deltas. Each new game
should reduce to "N missing symbols + M missing selectors", with per-game cost dropping.

## Open questions
1. Is on-screen League 16.15 the goal, or is headless replay sim a satisfying near-term
   milestone? (Determines how soon the Metal swamp matters.)
2. Is a GL-era `.app` archived / re-materializable from manifests? Keeps a rendering demo
   alive during phase 2.
3. Location of customlolskins-manager repo (source for the FAT parser port).
