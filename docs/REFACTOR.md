# macholo — stub split + runtime-vendoring plan (draft 2026-07-31)

Status: **draft for refinement.** Written after the Darling differential session + WC3
bring-up. Two independent tracks: **(1)** split the monolithic stub lib into
per-framework units (low-risk, do first), **(2)** decide whether to vendor a real
objc/CF instead of the current GNUstep stack (strategic; can wait). They are
decoupled — #1 lands regardless of how #2 is decided, and #1 makes #2 tractable.

Read alongside [LOADER.md](LOADER.md) (what the loader really does vs fakes) and
[PLAN.md](PLAN.md) (roadmap; phase 2/3/4). This doc does not restate those.

---

## 0. Current architecture (facts, verified against source)

- **One monolithic target**: `stub/CMakeLists.txt` builds a single `stub` SHARED lib
  from every `stub/src/**/*.{m,c,S}`. There are no internal boundaries — files are
  grouped only by directory (`appkit/`, `coregraphics/`, `misc/`, `system/`).
- **Resolution is one flat namespace.** `lib::resolve(name)` (`src/lib.cpp:255`):
  1. `dlsym(nullptr, "$darwin$" + name)` — the override lane; wins over everything
     (used e.g. to force all CF URLs onto one NSURL implementation).
  2. a small static `OVERRIDE[]` table — dyld/dl entry points (`_dyld_*`, `dladdr`,
     `dlopen`, `dyld_stub_binder`, `mach_vm_remap`, std stream pointers).
  3. a `BLOCKED[]` prefix list → `nullptr`.
  4. fallback `dlsym(nullptr, name)` — finds the symbol anywhere in the loader's
     global scope (the `stub` lib, gnustep libs, libc, …).
  `resolve_import` (`:311`) just strips a leading `_` and calls `resolve`.
- **Consequence that de-risks track 1:** because resolution is `dlsym(nullptr, …)`
  over the whole process, *how the HLE surface is split across `.so` files does not
  matter to the resolver* — every lib linked into (or `dlopen`ed `RTLD_GLOBAL` by)
  the loader contributes to the same global namespace. Splitting is a build change,
  not a resolver change.
- **objc/CF/Foundation today** = GNUstep: `libobjc2` (real nonfragile-ABI runtime) +
  `gnustep-base` (Foundation) + `gnustep-corebase` (CF), built into `deps/sysroot`
  via podman. Apple objc metadata in the loaded image is rewritten to the GNUstep
  ABI by the converter in `src/objc.cpp`. Hand-written stubs fill the framework
  surface on top.

### Known smells this refactor targets
- `system/stub_misc.c` is a **junk drawer**: CFNotificationCenter, Battle.net
  ClientSdk, raw sysctl, `malloc_size`, `__sincos_stret` all live together. It caused
  real friction cherry-picking WC3 work back to master (unrelated changes tangled in
  one file).
- `misc/Misc.m` is a second junk drawer (mixed CF/NS numeric + URL helpers).
- Game-specific stubs (`stub_riot.c`, ClientSdk) are compiled into the one lib that
  every game loads — no isolation, no per-game opt-in.
- gnustep-corebase is internally inconsistent (CFBundle returns real NSURLs;
  CFURL.c is a separate CFRuntime impl → NULL-deref when mixed; papered with
  `$darwin$` for WC3). See LOADER.md.

---

## Track 1 — split the monolith into per-framework libs

**Goal:** replace the single `stub` lib with a set of `libmacholo-<framework>` static
or shared libs, each owning one framework's surface, all linked into the loader (so
`dlsym(nullptr,…)` resolution is unchanged). Dissolve the two junk drawers. Isolate
game-specific stubs into opt-in units.

**Why:** legibility (per-game deltas become "which libs + which missing symbols"),
clean ownership for parallel work, and it is the prerequisite for swapping any one
framework's implementation (track 2) without touching the rest.

### Proposed module boundaries (from the current file tree)

| New lib | Absorbs (current files) | Notes |
|---|---|---|
| `appkit` | all `appkit/*.m` except SBApplication | Cocoa UI over SDL2 |
| `scriptingbridge` | `appkit/SBApplication.*` | not AppKit; tiny, own unit |
| `coregraphics` | `coregraphics/CG.*` | + CGDisplay/CGL surface |
| `cg` (Cg shader) | `system/stub_nvcg.c` | verify: NVIDIA Cg, not CoreGraphics |
| `corefoundation` | `misc/CFURL.m`, CF parts of `misc/Misc.m`, **CFNotificationCenter split out of `stub_misc.c`** | the CF surface; track-2 seam |
| `foundation` | NSUserDefaults-suite + NS numeric helpers from `misc/Misc.m` | mostly gnustep-base today |
| `carbon` | `system/stub_carbon.m` | TIS/UCKeyTranslate/FSRef |
| `security` | `system/stub_securetransport.c` | SecureTransport/trust (fail-clean) |
| `audiotoolbox` | `system/stub_audio.c` | AUGraph etc. |
| `objc-rt` | `system/stub_objc.c`, `system/stub_objc.S` | objc entry-point shims incl. `object_{get,set}InstanceVariable` ABI fix, personality, chkstk |
| `libsystem` | `stub_kernel*`, `stub_mach_vm`, `stub_pthread`, `stub_fs`, `stub_io`, `stub_socket`, `stub_sys`, `stub_str`, `stub_process`, `stub_resolv`, `stub_rune`, `stub_dispatch`, `stub_xpc`, `stub_asl`, `stub_math` | the mach/BSD/libc surface; consider sub-splitting mach vs BSD vs math later |
| `game-riot` (opt-in) | `system/stub_riot.c` | RPatch/mvg — League only |
| `game-blizzard` (opt-in) | **ClientSdk split out of `stub_misc.c`** | WC3 only. **Correction (2026-07-31): ClientSdk exists only on the `wc3-*` branches, not master — this module materializes when that work merges, following the `game-riot` pattern.** |

Residual `stub_misc.c` after extraction (sysctl/`malloc_size`/`__sincos_stret`) folds
into `libsystem` (math bits to the math unit). `stub_misc.c` and `Misc.m` cease to
exist as names.

### Staging (each stage independently buildable + League 14.8/16.3 regression-clean)
1. **Scaffold**: convert `stub/CMakeLists.txt` to `add_subdirectory` per module; each
   module an `OBJECT` or `STATIC` lib, all aggregated into one final lib (or linked
   directly into the loader) so the global namespace is preserved. No files move yet
   — just the build graph. Regression gate.
2. **Dissolve junk drawers**: split `stub_misc.c` and `Misc.m` into their target
   modules. This is the highest-value cleanup and the one that bit the cherry-pick.
3. **Isolate game-specific** (`game-riot`, `game-blizzard`) behind a build option /
   per-game link set; default builds may include all (harmless) but the *option to
   exclude* exists.
4. **Header hygiene**: per-module public headers; kill `common.h` catch-all if it
   fragments cleanly.

### Regression gate for every stage
`master`'s League 14.8 **and** 16.3 must reach `GAMESTATE_GAMELOOP` (both were
verified x4 on the current master). Run headless: `pgrep -x macholo` for liveness —
**never** pattern-match the game path (it collides with tool shells; this burned us).

### Worker-dispatch notes
- Stages 1→4 are sequential (each depends on the prior build graph) but *within*
  stage 2 the per-module extractions are independent → parallelizable across workers,
  one module each, each verified by a build + symbol-count diff (no dropped exports).
- Verification per worker: `nm`/`dlsym` the aggregate before/after must expose the
  **same** symbol set (a split must not drop or rename an exported symbol). A tiny
  `tools/exports-diff.sh` (dump defined symbols of the final lib) is worth writing
  first as the shared guardrail.

---

## Track 2 — vendor a real objc/CF? (strategic; decide, don't rush)

The question posed: keep GNUstep, or vendor an implementation the way Darling (ports
Apple's objc4/CF) and touchHLE (writes its own) do. Framed as three options plus
status quo. This track is **gated on nothing** — track 1 can complete first.

### The pain being weighed (from LOADER.md + this session)
- The **metadata converter** (`src/objc.cpp`) exists only because libobjc2's ABI ≠
  Apple's; it is load-time complexity and a source of subtle bugs (e.g. the
  `object_{get,set}InstanceVariable` semantics mismatch that crashed WC3 — fixed, but
  symptomatic).
- **gnustep-corebase is internally inconsistent** (CFURL vs CFBundle), already papered
  over with `$darwin$`. It is the shakiest dependency.
- gnustep-base pulls a large Foundation we only partly use.

### Options

**A. Status quo** — keep libobjc2 + gnustep-base + gnustep-corebase + converter.
Cost: 0. Risk: ongoing corebase inconsistency; converter friction per new game.
Verdict: fine as the floor; the two games run on it.

**B. Mini-CF as libobjc2 objects** *(recommended first move)* — replace **corebase
only** with a hand-written CF (~30 symbols, already sketched in PLAN.md phase 2/3)
implemented *as* libobjc2 objects, so CFRetain/CFRelease and toll-free bridging with
NSString/NSArray are coherent by construction. Deletes the corebase inconsistency and
the `$darwin$` CFURL hack. Keeps libobjc2 + gnustep-base + the converter.
Cost: medium, bounded (the symbol set is known from `docs/surface-*/CoreFoundation.txt`).
Risk: low — swaps the shakiest dep for owned code, leaves the proven runtime alone.
This is the pragmatic middle and the natural CF seam that track 1's `corefoundation`
module creates.

**C. Own objc runtime (touchHLE-style)** — consume Apple's objc ABI directly, delete
the converter. ~20 entry points + dispatch + class tables. Deletes the most
"magic" component, but re-solves exceptions interop, ARC, root-class behavior.
Cost: high. Verdict: only if libobjc2 *itself* becomes the pain (it hasn't).

**D. Port Apple CF/Foundation (Darling-style)** — heaviest, drags Apple's whole stack
and its bugs (we just spent a session cataloguing them). Verdict: no; our own
Darling notes are the argument against.

### Recommendation
Sequence **B after track 1**: track 1's `corefoundation` module is exactly the seam
to swap. Do **not** pursue C/D unless a concrete target forces it. Keep libobjc2.
Re-evaluate C only if a game needs objc behavior the converter can't express.

### Open questions for the refining session
- Exact mini-CF symbol set: intersect `docs/surface-16.3/CoreFoundation.txt`,
  `surface-16.15/`, and WC3's `--syms` CF imports → the real required surface.
- Does dropping gnustep-corebase strand any gnustep-base internals that call into it?
  (Audit gnustep-base → corebase edges before committing to B.)
- Toll-free bridging depth actually exercised (which CF types must bridge vs which are
  used opaquely) — sizes B precisely.

---

## Suggested overall order
1. Track 1 stage 1–2 (scaffold + dissolve junk drawers) — pure win, unblocks everything.
2. Track 1 stage 3–4 (game isolation + headers).
3. Track 2 option B (mini-CF) at the `corefoundation` seam.
4. Revisit C only on demand.

Every step: League 14.8 + 16.3 to GAMELOOP, headless, `pgrep -x macholo` liveness.

---

# Concrete execution plan (2026-07-31, branch `refactor/stub-split`)

Track 1 made concrete. Decisions below are settled unless a stage proves one wrong;
workers execute, they don't re-litigate.

## Decisions

1. **Module form: `OBJECT` libraries, aggregated into the single `stub` SHARED lib.**
   Layout: `stub/src/<module>/CMakeLists.txt` + `add_subdirectory` per module;
   `stub/CMakeLists.txt` keeps `add_library(stub SHARED)` linking all module objects.
   OBJECT libs cannot drop unreferenced symbols (no `--whole-archive` games), so the
   exported surface is bit-identical by construction. Loader/resolver untouched.
2. **Arch quarantine (the aarch64-Android enabler): new module `arch-x86_64`.**
   Absorbs `stub_objc.S` (msgSend/superclass thunks, `__objc_personality_v0`,
   `___chkstk_darwin`) and `__sincos_stret`/friends (x86 two-register-return ABI).
   Rule going forward: any code that mentions a register, an ABI struct-return
   convention, or `.S` lives here. An arm64 port adds `arch-aarch64/` beside it and
   the rest of the tree doesn't care.
3. **Module set** = the draft table above, plus corrections from the file audit:
   `iokit` (`stub_iokit.c` — IOKit registry/service surface, missing from the draft
   table) and `stub_video.c` is empty → **delete it**. `cg` module keeps draft name
   `stub_nvcg.c` pending the NVIDIA-Cg-vs-CoreGraphics verification.
4. **Game isolation**: `game-riot` + `game-blizzard` OBJECT libs behind
   `option(MACHOLO_GAME_STUBS "..." ON)` — default builds identical, exclusion possible.
5. **Guardrail**: `tools/exports-diff.sh` (in-tree; baseline = 614 dynamic exports of
   `libstub.so` at `5b6d32a`). Every stage must pass `check` — a refactor that adds or
   drops an export is wrong until explained (game modules staying in the default build
   keep even stage 3 diff-clean).
6. **Regression gate, pinned invocation** (the cwd part bit us: the game resolves
   `DATA/FINAL/…` and writes `../SOFT_REPAIR` relative to **cwd**, so run from
   `Contents/MacOS`):
   ```sh
   cd ~/Game/$V/LeagueofLegends.app/Contents/MacOS
   rm -f ../SOFT_REPAIR
   DISPLAY=:9 timeout 900 $REPO/build/macholo ./LeagueofLegends ~/Game/$V-*.rofl
   grep -c GAMESTATE_GAMELOOP <log>   # gate: >= 1, for V in 14.8 and 16.3
   ```
   (Xvfb `:9`, llvmpipe.) Full game gate after stages 1, 2, and 4; build +
   exports-diff after every worker.

## Stages → workers

| Stage | Work | Worker | Verify |
|---|---|---|---|
| 1 | Scaffold: per-module OBJECT libs + `add_subdirectory` graph, **files move to module dirs but are not split**; create `arch-x86_64` (moves `stub_objc.S`); delete `stub_video.c` | opus | build + exports-diff + full gate |
| 2a | Dissolve `stub_misc.c`: CFNotificationCenter→`corefoundation`, sysctl/`malloc_size`→`libsystem`, uncategorized leftovers→`libsystem/stub_compat.c` | sonnet | build + exports-diff |
| 2b | Dissolve `misc/`: `Misc.m` CF parts→`corefoundation`, NS parts→`foundation`; `CFURL.m`→`corefoundation` | sonnet | build + exports-diff |
| 3 | `MACHOLO_GAME_STUBS` option; `stub_riot.c`→`game-riot`; move `$darwin$__sincos{,f}_stret` from `libsystem/stub_math.c`→`arch-x86_64` (x86 two-register-return ABI); header hygiene (`common.h` is 27 lines — dissolve into module headers if it splits cleanly, else keep) | sonnet | build ×2 (ON+OFF) + exports-diff (ON) |
| 4 | — | supervisor | full gate, review, PR |

Stages sequential (2a→2b share module CMakeLists; not worth conflict risk).
Supervisor session runs gates between stages and keeps a second opus in reserve for
whatever breaks; it reviews diffs rather than re-doing work.

## Track 3 (proposed 2026-07-31) — explicit libsystem surface, kill the yolo lane

Input: real-Mac export dumps (`~/projects/osx-libs/macos-exports-26.6-arm64/`, both
arches + objc metadata; regenerate with `tools/mac-dump-exports.sh`) and the
[libsystem mapping](MACOS-26.6-SURFACE.md). Idea: today resolver lane 4
(`dlsym(nullptr, name)`) silently binds any unhandled Darwin symbol to the
same-named glibc symbol — correct for `memcpy`, quietly ABI-wrong for anything
struct/errno/off_t-shaped. Replace accident with audit:

1. Classify every game-imported libsystem symbol: explicitly stubbed / falls
   through to glibc / unresolved. The fall-through set is the audit set.
2. Split the audit set: safe-forward (identical ABI+semantics) vs needs a
   `$darwin$` shim, shims grouped into the per-module files from track 1
   (Apple's own `libsystem_*` sublibrary boundaries say which file).
3. Emit a checked-in forward allowlist the resolver consumes; lane 4 becomes
   deny-by-default (unknown import = loud bind-time error, not a glibc guess).

Also the natural home for bionic-vs-glibc deltas when the aarch64/Android track
starts. Sequencing: after the track-1 PR lands; classification is sonnet-shaped
work with the decision rules written down, ABI-divergence review is the part that
needs care.

## aarch64 / Android outlook (why the split is shaped like this)

Goal on record: an eventual **arm64 Android replay-playback client**. 16.x League
macOS builds are FAT and the loader already slices by host arch, so the entry cost is
the runtime, not the file format. What this refactor contributes: (a) all ISA/ABI
assembly behind the `arch-x86_64` seam, (b) per-framework libs so a phone client can
drop `appkit`/GL for a different presentation layer, (c) track-2B mini-CF would
remove the hardest cross-compile dependency (GNUstep on bionic).

Known gaps, explicitly **out of scope** now, kept as the porting checklist:
arm64 `objc_msgSend`-family thunks (`arch-aarch64`); arm64e/chained-fixup handling if
the arm64 slice uses them; comm-page + TLV arm64 layout; bionic-vs-glibc deltas in
`libsystem`; GNUstep-on-Android (or mini-CF first); SDL2 Android backend for the
window/GL seam; verify anon-exec-page policy for untrusted apps on target Android
versions.
