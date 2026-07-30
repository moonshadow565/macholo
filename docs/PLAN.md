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
- [x] FAT header slicing (`macho::find_slice`, cslol-manager–style); selects slice by
      host arch, loader maps at slice offset. Verified on 14.8 (thin) + 16.3/16.15 (FAT).
- [x] Section search across **all** segments — `find_section()` (né `find_data_section`)
      now finds `__mod_init_func`/`__objc_classlist`/`__cfstring` in `__DATA_CONST`.
- [ ] Audit tool: `macholo-dump` (src/dump.cpp) parses + prints segments/binds with
      counts matching llvm-objdump; still to add: per-dylib attribution, objc class/sel
      surface, diff against implemented stubs (`docs/surface-*/` are the target output).
- [x] Podman dep build (`deps/` — see [GNUSTEP-BUILD.md](GNUSTEP-BUILD.md) for the
      rationale and ABI landmines). Host build works on Fedora 44:
      `CC/CXX/OBJC/ASM=clang cmake -B build -DGNUSTEP_DIR=$PWD/deps/sysroot/usr/GNUstep`
      (+ `libstdc++-static` for `-lsupc++`).

### Phase 1 — regression baseline on 14.8 ✅ (2026-07-30)
Thin x86_64, no `__DATA_CONST`, GL + Cg renderer, only Cg.framework hard-linked
(RiotGamesApi/RPatch/mvg arrive via dlopen, which the stubbed-to-fail dlopen already
handles). Replay: `14.8-NA1-4982225995.rofl`.
- [x] **Milestone reached**: replay runs headless to `GAMESTATE_GAMELOOP` and renders
      frames — Xvfb + llvmpipe (GL 3.2 core over Mesa 26), Wwise inits through the ALSA
      sink (`~/.asoundrc` null default on soundless machines), full flow
      Bootstrap → Patching → LoLCommon → PREGAME → SPAWN → GAMELOOP. ~1–4 s/frame on
      llvmpipe (4-core VM) — fine for correctness work.
- Env quirks worth remembering: game chdirs to `Contents/MacOS` and resolves
  `DATA/FINAL/...` from there → needs `DATA` symlink inside `Contents/MacOS` (or the
  proper GameBaseDir argument, to be found); it writes a `SOFT_REPAIR` marker on failed
  wad mounts — delete it between runs.
- Remaining loose ends (small): unresolved `NSProgressIndicator`/`NSImageView` stub
  classes, `kSec*` Security constants, `_backtrace_from_fp`; NSAlert stub panics instead
  of printing the alert text (it fires on real error paths — make it log+dismiss).

### Phase 2 — primary target: 16.3 renders on screen ✅ (2026-07-31)
**Milestone reached**: `16.3-EUN1-3908472567.rofl` plays to `GAMESTATE_GAMELOOP`,
rendering on llvmpipe (`X3D Platform: OpenGL 3.2 Core`), zero unresolved symbols
except the intentional weak Metal set. 14.8 regression clean.
- [x] FAT + `__DATA_CONST` (phase 0) — plus a third loader fix found here: `__DATA`'s
      zerofill extends past the file slice, so the loader now reserves the full VM span
      anonymously and overlays the file with `MAP_FIXED` (14.8 survived by accident).
- [x] Modern objc entry-point shims (`objc_opt_*`; libobjc2 v2.3 already exports
      `objc_alloc`/`objc_alloc_init`), `__objc_personality_v0` → libobjc2,
      `___chkstk_darwin`.
- [x] ~~Multi-image loader~~ **Not needed**: stubbing the 18 RPatch/mvg imports
      (patcher returns "nothing to do", telemetry disconnected) satisfies replays;
      RiotGamesApi has zero static imports. Multi-image loading moves to the
      "more games" phase if a target actually calls into a bundled dylib.
- [x] Renderer selection: with Metal resolved to NULL the game selects GL on its own.
- Two ABI-level discoveries with impact beyond 16.3 (see commit messages):
  **tlv.cpp never called `pthread_setspecific`** — TLV writes never stuck; in 16.3
  this livelocked Async Loading via a TLS bump allocator degrading to O(n²) with
  unbounded chunk leaks. And **Darwin `%gs` TSD emulation** (`arch_prctl(ARCH_SET_GS)`
  per thread) — inlined `mov %gs:N` accesses fault on Linux otherwise.
- [ ] Mini-CF (~30 syms, `surface-16.3/CoreFoundation.txt`) — still planned, but as
      cleanup (gnustep-corebase currently covers it), no longer a blocker.

### Phase 3 — mini-Foundation, drop gnustep-base
Sized by phase-1/2 selector logs. Expect ~15 classes, shallow usage (paths, bundle,
process info, strings).

### Phase 4 — Metal HLE (only for 16.4+ / staying current with live patches)
16.15 is Metal-only: `MTLCreateSystemDefaultDevice` + 8 descriptor classes +
CAMetalLayer; device/queue/encoder protocols flow through msgSend (invisible statically —
enumerate via dispatch logging on 16.3, which drives the same Metal path when available).
- [x] **Shader-format spike (2026-07-31): precompiled AIR, best case.** All three
      versions ship `ShaderCache.metal.wad.client` entries as standard `MTLB` metallib
      containers (Riot wrapper is just `u32 LE length + blob` chunks plus `TOC3.0`
      permutation tables; ~60–74k metallibs per version). Universally single-function
      (`wrapped_main`), vertex/fragment only (no compute), MSL 1.1 / AIR 1.8, compiled
      with Apple's Metal tools for Windows; unencrypted, and **stock LLVM 22 `llvm-dis`
      parses the bitcode**. 16.15 ships no GLSL wad at all (GL removal confirmed).
      Extraction: cslol-tools `wad-extract` builds standalone on Linux
      (`-DCMAKE_POLICY_VERSION_MINIMUM=3.5`); tools + `hashes.game.txt` in
      `~/.local/bin`.
- [x] **Iridium gate spike (2026-07-31): not viable as-is; usable as a skeleton.**
      Census over all 206,412 metallibs (193,263 unique — dedupe only 6%; full data:
      206k-module census, 83 distinct `air.*` intrinsics, uniform AIR 1.8/MSL 1.1,
      no compute/atomics/threadgroup/fp16; texture surface includes array/3D/depth-
      compare/integer reads and derivatives). Iridium (in `~/dev/indium`, builds only
      against **LLVM 14** — five `LLVMGetElementType` sites break under opaque
      pointers) implements 13/83 intrinsics, and the bigger gap is missing LLVM
      *instruction* coverage (`select`/`icmp`/`alloca`/int-ALU…): stratified 510-module
      `mtl2spv` trial → **0.59% pass spirv-val**. Read: its metadata/binding/sampler
      handling is real, but the instruction selector + texture-type layer need
      rewrite-scale work, plus either an LLVM-14 pin or an opaque-pointer port.
      Decision if/when phase 4 starts: extend Iridium as skeleton vs from-scratch
      targeted AIR→SPIR-V — comparable effort either way against this uniform corpus;
      budget months. Caveat to carry: every shader sets `fast_math_enable` — map
      deliberately onto SPIR-V relaxed-math or expect visual drift.
- [ ] Mini-Metal runtime facade over Vulkan (lavapipe works headless, Vulkan 1.3 +
      timeline semaphores required by indium). Demoted from the critical path by 16.3's
      GL renderer.

### Phase 5 — more games
Pick 2–3 macOS-only x86_64 titles; run the audit tool; implement deltas. Each new game
should reduce to "N missing symbols + M missing selectors", with per-game cost dropping.

## Open questions
1. Location of customlolskins-manager repo (source for the FAT parser port).
2. ~~Is a GL-era `.app` archived?~~ **Answered: 16.3 (and 14.8) in `~/Game/`.** On-screen
   rendering no longer depends on Metal HLE.
