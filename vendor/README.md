# Vendored dependencies

Everything here builds from the top-level `cmake -B build -G Ninja` with no network
access and no `deps/sysroot`. Subtrees are `git subtree --squash` pins; "frozen
config" means upstream's configure was run once per platform and its generated
headers committed (see each dir), per the hardcode-what-we-know policy.

| Dir | Upstream | Pin | Build | Notes |
|---|---|---|---|---|
| `libobjc2/` | gnustep/libobjc2 | v2.3 | upstream CMake + `libobjc2-fixup.cmake` | `.m`/`.mm` language pinned in `CMakeLists.txt` here — see the landmine comment |
| `gnustep-base/` | gnustep/libs-base | base-1_29_0 | `gnustep-base-cmake/` (hand-written) | one delta vs upstream: GSXML libxml2≥2.12 compat. Slimmed: curl/gnutls/gmp/xslt/ICU disabled (runtime trace: zero bindings from our stack in a full boot-to-GAMELOOP run; ICU contributed two converter-name lookups with built-in fallbacks) |
| `gnustep-base-config/` | — | — | frozen configure output | per-platform; `x86_64-linux-gnu` only so far; aarch64 slots in beside it |
| `corebase-headers/` | gnustep/libs-corebase | b85d480 | headers only | types for mini-CF; corebase itself is deleted |
| `robin-map/` | Tessil/robin-map | v1.3.0 | INTERFACE | kills libobjc2's configure-time FetchContent |
| `libdispatch/` | swift-corelibs-libdispatch | swift-6.0.2-RELEASE | upstream CMake | also provides `libBlocksRuntime`; the guest's `dispatch_*` forwards resolve here |
| `libxml2/` | GNOME/libxml2 | v2.12.10 | upstream CMake | pinned to the Fedora version it replaced; minimal feature set |
| `libffi/` | libffi/libffi | v3.5.2 | `libffi-cmake/` frozen-configure | x86_64 sources + asm |
| `SDL2/` | libsdl-org/SDL | release-2.32.10 | upstream CMake | replaces Fedora's SDL3-backed sdl2-compat shim; `MACHOLO_SYSTEM_SDL=ON` for system SDL |

Deliberately **not** vendored (the platform seam): glibc/libm/libgcc_s, libc++/libc++abi,
libGL/Mesa, ALSA, zlib (universal, stable ABI, NDK ships it).
