# Why the GNUstep deps are built the way they are

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
- `-lsupc++` in the root CMakeLists needs `libstdc++-static` on Fedora 44 (the 64-bit
  `libsupc++.a` lives there; gcc-c++ only ships the 32-bit one). Known cleanup: linking
  both libc++abi *and* libsupc++ means two competing Itanium-ABI runtimes — it works
  because libc++abi wins, but `-lsupc++` should eventually just be dropped.
- Known duplicate-symbol wart, inherited from the old tree: both libobjc.so and
  libBlocksRuntime.so (via libdispatch) export `_Block_copy`; first in scope wins and the
  smoke test passes. First suspect if block-related weirdness ever appears.
- Headless runtime: Xvfb + Mesa llvmpipe provides GL 4.6 (the 14.8/16.3 GL renderer runs
  on it, slowly), and `~/.asoundrc` with `pcm.!default { type null }` satisfies the Wwise
  ALSA sink's `snd_pcm_open("default")` on machines with no sound hardware.
