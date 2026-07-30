# macholo — roadmap (2026-07)

Decision: **evolve macholo, do not restart.** The parser/loader/objc-converter/TLV core is
sound and proven; the pain is the implicit dependency swamp (system-installed GNUstep tree,
prebuilt tarball, apt-based build script) and a few loader shortcuts. Those get replaced;
the code doesn't.

Reference targets, all in `~/Game/` with matching `.rofl` replays
(see [AUDIT-VERSIONS.md](AUDIT-VERSIONS.md) for the full comparison):

- **14.8** — thin x86_64, GL + Cg renderer, no `__DATA_CONST`. The era macholo last ran;
  regression baseline.
- **16.3** — FAT, `__DATA_CONST`, modern objc entry points, **still has the full OpenGL
  renderer** (~111 OpenGL.framework binds) next to weak-linked Metal. **Primary target**:
  exercises every loader modernization while rendering through the existing SDL/GL stubs.
- **16.15** — same as 16.3 but Metal-only (GL renderer removed between 16.3 and 16.15).
  Needs Metal HLE; deferred.

Exact import surfaces: [surface-16.3/](surface-16.3/), [surface-16.15/](surface-16.15/).

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
on the right side of this line. Games genuinely don't need much: League's entire
non-libc/libc++ surface is ~135 framework symbols + ~30 objc classes (plus ~111
direct-mapped gl* functions on 16.3).

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
- [ ] Section search across **all** segments — `__DATA_CONST` (16.3+) holds
      `__mod_init_func`, `__objc_classlist`, `__objc_protolist`, `__cfstring`;
      `find_data_section()` only searches `__DATA`, so initializers + objc registration
      silently no-op on new binaries.
- [ ] Audit tool: `macholo --audit <binary>` → import surface (per-dylib symbols, objc
      classes/sels) diffed against implemented surface. Turns "will game X run?" into a
      printed to-do list. Parsing already exists; `docs/surface-*/` are manual prototypes
      of its output.
- [ ] Podman dep build; `cmake -B build && cmake --build build` works on Fedora 44.

### Phase 1 — regression baseline on 14.8
Thin x86_64, no `__DATA_CONST`, GL + Cg renderer, only Cg.framework hard-linked
(RiotGamesApi/RPatch/mvg arrive via dlopen, which the stubbed-to-fail dlopen already
handles). This is the client generation macholo actually ran — the goal is proving the
rebuilt dep environment (podman libobjc2 + gnustep-base) and the existing stubs still
work, before layering new features. Replay: `14.8-NA1-4982225995.rofl`.
- Milestone: replay renders via the existing NSOpenGLContext/SDL/GL path.

### Phase 2 — primary target: 16.3 renders on screen
Everything modern except the renderer swamp. Replay: `16.3-EUN1-3908472567.rofl`.
- [ ] FAT + `__DATA_CONST` (from phase 0).
- [ ] Modern objc entry-point shims: `objc_alloc`, `objc_alloc_init`, `objc_opt_class`,
      `objc_opt_isKindOfClass`, `objc_opt_new`, `objc_opt_self`, `objc_msgSendSuper2`
      (non-fixup). The old `*_fixup` variants in `stub_objc.S` are no longer referenced.
- [ ] Multi-image loader: 16.3 hard-links RPatch/mvg/RiotGamesApi via `@rpath` (12+6
      lazy binds land in bundled frameworks). Per-image rebase/bind/objc/TLV,
      cross-image export resolution.
- [ ] Mini-CF (~30 syms, `surface-16.3/CoreFoundation.txt`) incl.
      `__CFConstantStringClassReference` and `CFPropertyListCreateFromXMLData`.
- [ ] Renderer selection spike: Metal is weak-linked, OpenGL hard-linked — resolve
      Metal imports to NULL and verify the game falls back to GL (expected, since weak
      linkage exists to run on Metal-less configurations); if it doesn't, find the
      renderer config knob or patch selection.
- [ ] The ~111 `gl*` imports map 1:1 to host libGL; only CGL*/NSOpenGL* need stubs
      (mostly existing).
- Milestone: modern-client replay renders on Linux without any Metal work.

### Phase 3 — mini-Foundation, drop gnustep-base
Sized by phase-1/2 selector logs. Expect ~15 classes, shallow usage (paths, bundle,
process info, strings).

### Phase 4 — Metal HLE (only for 16.4+ / staying current with live patches)
16.15 is Metal-only: `MTLCreateSystemDefaultDevice` + 8 descriptor classes +
CAMetalLayer; device/queue/encoder protocols flow through msgSend (invisible statically —
enumerate via dispatch logging on 16.3, which drives the same Metal path when available).
- [ ] Spike: what does the game feed Metal — runtime MSL source
      (`newLibraryWithSource:`) or precompiled `.metallib` (AIR bitcode)? MSL source →
      constrained translator over Riot's shader corpus is tractable; metallib → LLVM AIR
      parsing, much harder.
- [ ] Mini-Metal over Vulkan (or GL 4.6) as another stub framework. Prior art to study,
      not adopt: Darling's `indium`. Months, not weeks — demoted from the critical path
      by 16.3's GL renderer.

### Phase 5 — more games
Pick 2–3 macOS-only x86_64 titles; run the audit tool; implement deltas. Each new game
should reduce to "N missing symbols + M missing selectors", with per-game cost dropping.

## Open questions
1. Location of customlolskins-manager repo (source for the FAT parser port).
2. ~~Is a GL-era `.app` archived?~~ **Answered: 16.3 (and 14.8) in `~/Game/`.** On-screen
   rendering no longer depends on Metal HLE.
