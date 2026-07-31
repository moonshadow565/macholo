# Why the GNUstep deps are built the way they are

**As of 2026-07-31 the deps are vendored in-tree and built by macholo's own CMake — see
"The vendored build" below. `deps/Containerfile` + `deps/build-sysroot.sh` are
DEPRECATED**: nothing in a default build reads `deps/sysroot/` any more. They stay in the
tree for now because they are the provenance of the frozen configure output and the
reference the vendored build is diffed against; removing them is a separate decision.
Everything from here to "The vendored build" describes that podman flow and the ABI
reasoning behind it — the reasoning is still binding, the mechanism is not.

`deps/Containerfile` + `deps/build-sysroot.sh` produce `deps/sysroot/` — a reproducible
replacement for the old `stub/gnustep/GNUStep.tar.xz` (Ubuntu-built, unusable on Fedora:
linked libicu*.so.70) and `build-gnustep.sh` (apt-based, implicit system state). Built
2026-07 on Fedora 44, clang 22; component pins live in the Containerfile and
`sysroot/usr/GNUstep/BUILD-INFO.txt`.

## What gets built, and what deliberately doesn't

| Component | Why |
|---|---|
| libobjc2 (v2.3, soname libobjc.so.4.6) | The ObjC runtime. Only clang can target it (`-fobjc-runtime=gnustep-2.1`). |
| tools-make (make-2_9_3) | Build system for libs-base/corebase; `--with-library-combo=ng-gnu-gnu`. |
| libs-base (base-1_29_0) | Foundation; exports `GSInitializeProcess` which `loader.cpp` calls before the game's entry. Same version as the old known-good tree; builds fine with clang 22. |
| libs-corebase (master pin) | CoreFoundation for the CF symbols the stubs forward. Upstream's last *tag* is from 2013, so a pinned master commit is the only sane choice. Slated for replacement by mini-CF (PLAN.md). |
| libdispatch | From Fedora's `libdispatch` RPM (swift-corelibs 6.0.2) instead of a source build — ships dispatch headers, `Block.h`, `libBlocksRuntime.so`, and passes libs-base's link probe. One less component to build. |
| ~~libs-gui / libs-back / corebase-from-Hurd~~ | Not built. Nothing links them — `stub/` implements AppKit itself on SDL2. |

## The load-bearing decisions

- **`-fobjc-runtime=gnustep-2.1` (nonfragile ABI) is the point of the whole exercise.**
  It gives indirected class/ivar layout the same way Apple's modern runtime does, which is
  what makes `objc.cpp`'s conversion of Mach-O ObjC metadata into live GNUstep classes
  tractable. Requires clang; GCC cannot emit this ABI at all.
- **libc++ + libc++abi, not libstdc++** — matching macOS, whose C++ stdlib *is*
  libc++/libc++abi. libobjc2 implements the ObjC↔C++ exception interop, so its C++ ABI
  side should match what the loaded Mach-O code was compiled against. Note scope: libobjc2
  is the *only* dep containing C++; libs-base/corebase are pure C/ObjC and don't care.
- **The ABI landmine (cost a debugging session; don't regress it):** libobjc2 needs
  `-stdlib=libc++` in **both** `CMAKE_CXX_FLAGS` and `CMAKE_OBJCXX_FLAGS`. With only the
  former, `arc.mm` (ObjC++) compiles against libstdc++ headers and libobjc.so ships an
  unsatisfiable `std::__throw_length_error(char const*)`, breaking every later `-lobjc`
  link — including libs-base's configure, far from the actual cause.
- **Fedora 44 container base** so the sysroot's dependencies (ICU, gnutls, …) are the
  host's own sonames — `deps/host-packages.txt` is the exact runtime package list, and it
  was proven complete by running the smoke test in a container containing only those
  packages. This is precisely what the old Ubuntu tarball got wrong on Fedora.
- **`deps/smoke.m`** compiles+runs at image build time with the exact consumer flags from
  `stub/gnustep/CMakeLists.txt`: constant strings, blocks, CFString/CFArray, and
  `@throw`/`@catch` across the libobjc2↔libc++abi unwinder. If a future pin bump breaks
  ABI, the image fails to build instead of macholo failing at runtime.

## Patches / workarounds (in the Containerfile)

1. **libs-base GSXML.m**: drop the `ret->checked` access — libxml2 ≥ 2.12 removed
   `xmlEntity.checked` (recursion is tracked internally now). Still broken upstream, so a
   newer libs-base tag wouldn't help.
2. **clang 22 strictness**: demote `incompatible-pointer-types`, `int-conversion`,
   `implicit-function-declaration`, `implicit-int`, `incompatible-function-pointer-types`
   from errors to warnings for the autotools builds; without this libs-base's
   `-fconstant-string-class` configure probe fails and configure aborts with a misleading
   error.
3. **libs-corebase configure** needs explicit `CPPFLAGS`/`LDFLAGS` for the GNUstep tree
   on top of `gnustep-config --objc-flags`, or it dies with "Objective-C library not
   found!".

## Dropped from the old build script (reference: stub/gnustep/build-gnustep.sh)

- `-std=c++11` — libobjc2 v2.3 requires C++17; the pin was actively harmful.
- `-fuse-ld=lld` for the deps — default linker works; irrelevant to the produced ABI.
- source-built libdispatch + `PKG_CONFIG_PATH=/usr/local/...` — replaced by the RPM.

## Host-side notes (consumer = macholo's own CMake)

- Configure with `CC/CXX/OBJC/ASM=clang` (ASM too — `stub_objc.S` uses clang-only flags)
  and `-DGNUSTEP_DIR=$REPO/deps/sysroot/usr/GNUstep`.
- Runtime packages: `sudo dnf install $(cat deps/host-packages.txt)`.
- The root CMakeLists links `-lc++abi` only. It used to also carry `-lsupc++` (inherited
  from the old build script), which meant two competing Itanium-ABI runtimes in one link —
  harmless only because libc++abi won symbol resolution — and dragged in a
  `libstdc++-static` build dependency for the 64-bit `libsupc++.a`. **Dropped 2026-07**:
  the link is clean, and both the 14.8 and 16.3 replays still reach `GAMESTATE_GAMELOOP`,
  which exercises the ObjC↔C++ exception interop end to end. `libstdc++-static` is no
  longer needed to build macholo. `libstdc++.so.6` still shows up in `ldd`, but only
  transitively via libicu — nothing in the tree links it directly.
- Known duplicate-symbol wart, inherited from the old tree: both libobjc.so and
  libBlocksRuntime.so (via libdispatch) export `_Block_copy`; first in scope wins and the
  smoke test passes. First suspect if block-related weirdness ever appears.
- Headless runtime: Xvfb + Mesa llvmpipe provides GL 4.6 (the 14.8/16.3 GL renderer runs
  on it, slowly), and `~/.asoundrc` with `pcm.!default { type null }` satisfies the Wwise
  ALSA sink's `snd_pcm_open("default")` on machines with no sound hardware.

---

## The vendored build (2026-07-31)

The sysroot is gone from the default path. `vendor/` holds the sources and macholo's own
CMake builds them, so `cmake -B build -G Ninja` with `CC/CXX/OBJC/ASM=clang` is the whole
story — no podman, no install step, no `GNUSTEP_DIR`.

| Path | What | Pin |
|---|---|---|
| `vendor/libobjc2` | git subtree `--squash` of gnustep/libobjc2 | `v2.3` = `e877e782fb965c4e870d2ecf6aff58dddc6290ae` |
| `vendor/gnustep-base` | git subtree `--squash` of gnustep/libs-base | `base-1_29_0` = `c6af8ef4bf44c2d3ed36a136f1799fe263978af4` |
| `vendor/gnustep-base-cmake/` | hand-written CMake for gnustep-base | — |
| `vendor/gnustep-base-config/x86_64-linux-gnu/` | frozen `configure` output | — |
| `vendor/corebase-headers/` | 18 CoreFoundation headers for mini-CF | libs-corebase `b85d48066ba0fdb1333f94776cef122cdfba007e` |

Same pins the Containerfile used. **tools-make is not vendored** — it existed only as
gnustep-base's build system, and there is no longer a gnustep-make build. libs-corebase
contributes headers only; no corebase code and no corebase library survive.

The vendored trees are byte-identical to upstream at those commits except for one commit
against `vendor/gnustep-base/Source/Additions/GSXML.m` (the libxml2 ≥ 2.12
`xmlEntity.checked` removal, patch #1 above). `git log -- vendor/gnustep-base` shows it.

### Why there is no autoconf port

gnustep-base's `configure` answers ~900 `HAVE_*`/`SIZEOF_*` questions. Re-asking them from
CMake would be a large, low-value, drift-prone port. Instead the generated headers were
captured once from a real Containerfile build and committed per platform:

- `include/GNUstepBase/GSConfig.h` — public, byte-identical to what the sysroot installs.
- `private/config.h` — the internal answer sheet; `Source/common.h` pulls it in as
  `"config.h"`.
- `private/dynamic-load.h` — configure picks the `dlopen` backend on linux-gnu; forwards
  to the vendored `Source/simple-load.h`.
- `Resources/Info-gnustep.plist`, plus `config.mak` / `base.make` kept purely as
  provenance for what configure decided.

Include order puts the frozen dirs ahead of `vendor/gnustep-base/Headers`, so nothing in
the vendored tree needs editing. Porting to another triple means re-running the
Containerfile there and adding a sibling directory; nothing else is platform-conditional.

### Equivalence to the sysroot build, measured

The source list, the three per-subproject flag sets (`Source`, `Source/Additions`,
`Source/unix`) and the link line in `vendor/gnustep-base-cmake/CMakeLists.txt` are
transcribed from `make messages=yes` output of Containerfile step 3. Result:

- `libgnustep-base.so.1.29.0`, soname `libgnustep-base.so.1.29` — **DT_NEEDED identical**,
  **3805 exported symbols, zero diff**, undefined set identical.
- `libobjc.so.4.6`, soname `libobjc.so.4.6` — DT_NEEDED identical (`libc++.so.1` +
  `libc++abi.so.1`, no libstdc++). Exports differ only in two private `tsl::robin_map`
  symbols, because the host uses Fedora's `robin-map-devel` 1.3.0 while upstream's
  `FetchContent` fallback pulls 1.4.0. Not ABI-visible.
- `libstub.so` — `tools/exports-diff.sh check` unchanged against the committed baseline.

Gates: 16.3 and 14.8 both reach `GAMESTATE_GAMELOOP` (strict resolve, the default).

### Trimmed

Not built, because nothing links them: `Tools/` (autogsdoc, defaults, gdnc, gdomap, `pl*`,
xmlparse, HTMLLinker), `Tools/make_strings`, `Tests/`, `Examples/`, `Documentation/`, the
`win32` subproject, and the `ObjectiveC2` compatibility subproject (gnustep-make skipped
that one too — `OBJC2RUNTIME=1` under libobjc2). **No library source was dropped**: all 248
objects that went into the sysroot's `libgnustep-base.so` are built here, so the enabled
feature set is unchanged.

### Runtime resources: none are reachable, by design and by measurement

gnustep-base ships `Resources/` (NSTimeZones, `Languages/`, `*.lproj` strings, GSTLS) that
`+[NSBundle bundleForLibrary:]` looks for under `NSStandardLibraryPaths()`. Those paths come
from `/etc/GNUstep/GNUstep.conf`, or — absent that — from the paths compiled into
`config.h`, which are `/usr/GNUstep/{System,Local}/...`.

On dev-f44 neither exists: there is no `/etc/GNUstep`, and the sysroot was never installed
to `/usr/GNUstep`. `strace` on a 16.3 run to `GAMESTATE_GAMELOOP` shows exactly that — the
process probes and gets `ENOENT` on every one, and opens nothing:

```
access("/etc/GNUstep/GNUstep.conf", R_OK)                        = -1 ENOENT
access("/home/user/.GNUstep.conf", R_OK)                         = -1 ENOENT
newfstatat("/usr/GNUstep/{Local,System}/Library/Libraries", ...)  = -1 ENOENT
newfstatat(".../gnustep-base/Versions/1.29/Resources/Languages/English", ...) = -1 ENOENT
access("/etc/GNUstep/GlobalDefaults.plist", R_OK)                = -1 ENOENT
```

The only GNUstep path it *does* touch successfully is `~/GNUstep/Defaults`, the
NSUserDefaults user domain, which gnustep-base creates itself at runtime — not a shipped
resource.

Those paths are `config.h` constants, frozen from the same configure run the sysroot was
built from, so the **sysroot build never found these resources either**. Nothing was
replicated, and that is not a regression — it is the pre-existing behaviour, unchanged. If
a future game does need them, the fix is `GNUSTEP_CONFIG_FILE` (the env-var override is
enabled: `OPTION_NO_ENVIRONMENT 0`) plus a generated `GNUstep.conf`, not a file copy.

### Host build packages

Building the deps from source needs the `-devel` packages the container had. On top of
`deps/host-packages.txt` (runtime):

```
sudo dnf install clang lld cmake ninja-build \
    libcxx-devel libcxxabi-devel libdispatch-devel robin-map-devel \
    libffi-devel libxml2-devel libxslt-devel gnutls-devel libicu-devel \
    libcurl-devel gmp-devel zlib-devel openssl-devel
```

`robin-map-devel` is the only genuinely new one: libobjc2 needs `tsl::robin_map` and falls
back to a `FetchContent` git clone from GitHub if `find_package(tsl-robin-map)` fails, which
would make a fresh configure need the network. The RPM keeps configure offline.

### Escape hatch

`-DGNUSTEP_DIR=$REPO/deps/sysroot/usr/GNUstep` still works and skips `vendor/` entirely.
Its default changed from `/usr/GNUstep` to empty; empty is what selects the vendored build.

### Two CMake traps worth remembering

- **`.m` language flip.** C and OBJC both claim the `.m` extension. A *second*
  `cmake -B <existing-dir>` can flip `.m` from OBJC to C, silently dropping every
  `$<COMPILE_LANGUAGE:OBJC>` flag — including `-fobjc-runtime=gnustep-2.1`. clang still
  compiles `.m` as Objective-C, so it surfaces as a fragile-ABI `#error` deep in
  `GSVersionMacros.h`. `vendor/gnustep-base-cmake/CMakeLists.txt` pins `LANGUAGE OBJC` per
  source to make it deterministic.
- **libobjc2 assumes it is the top-level project.** Its `eh_trampoline.S` rule uses
  `CMAKE_SOURCE_DIR`/`CMAKE_BINARY_DIR` where it means its own directories, which breaks
  under `add_subdirectory`. `vendor/libobjc2-fixup.cmake`, injected via
  `CMAKE_PROJECT_libobjc_INCLUDE`, repairs it from inside libobjc2's own scope (`project()`
  resets those variables, so a parent-scope `set()` does not survive). The vendored
  `CMakeLists.txt` stays byte-identical to v2.3.
