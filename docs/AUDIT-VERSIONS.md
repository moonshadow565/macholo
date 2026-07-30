# Version comparison — League 14.8 / 16.3 / 16.15 (2026-07-30)

Three client generations in `~/Game/`, each with a matching replay. Detailed 16.15
analysis in [AUDIT-16.15.md](AUDIT-16.15.md); import surfaces in `surface-16.3/` and
`surface-16.15/`. All facts below are from the x86_64 slices via llvm-otool/llvm-objdump.

| | 14.8 (2024) | 16.3 | 16.15 |
|---|---|---|---|
| Build | 14.8.5756478 | 16.3.7457600 | 16.15.8008073 |
| Arch | **thin x86_64** | FAT x86_64+arm64 | FAT x86_64+arm64 |
| Fixups | LC_DYLD_INFO_ONLY | LC_DYLD_INFO_ONLY | LC_DYLD_INFO_ONLY |
| Version cmd | LC_VERSION_MIN_MACOSX | LC_BUILD_VERSION (sdk 26, minos 10.15) | LC_BUILD_VERSION (sdk 26, minos 10.15) |
| `__DATA_CONST` | **no** | yes | yes |
| objc entry points | `objc_alloc`, `objc_msgSendSuper2` only | + full `objc_opt_*`, `objc_alloc_init` | same as 16.3 |
| Renderer | **GL + Cg** + weak Metal | **GL** + weak Metal (Cg dropped) | **Metal only** |
| OpenGL.framework binds | 103 | 111 | 0 |
| Bundled frameworks | Cg, mvg, RiotGamesApi, RPatch | mvg, RiotGamesApi, RPatch | same as 16.3 |
| Bundled linkage | only **Cg** hard-linked (`@executable_path`); rest via dlopen | RPatch/mvg/RiotGamesApi hard-linked `@rpath` (12+6 lazy binds) | same as 16.3 |
| Replay | 14.8-NA1-4982225995.rofl | 16.3-EUN1-3908472567.rofl | EUN1-3984273452.rofl |

## What this means

- **14.8 is the era macholo last ran**: thin binary, no `__DATA_CONST`, Cg shaders
  (explains `stub_nvcg.c`), bundled deps reached only through dlopen (explains why the
  stubbed-to-fail dlopen was sufficient). Ideal regression baseline — should run with
  essentially the existing code once the dep environment is rebuilt.
- **16.3 is the sweet spot**: it requires every loader modernization (FAT,
  `__DATA_CONST`, modern objc entry points, multi-image loading) but still contains the
  complete OpenGL renderer — ~100 core-profile `gl*` functions that map 1:1 onto host
  libGL, plus NSOpenGLContext/NSOpenGLPixelFormat which the stubs already implement.
  Metal is weak-linked in all three versions while OpenGL is a hard link → resolving
  Metal to NULL on 16.3 should select the GL path (verify at runtime).
- **The GL renderer was removed between 16.3 and 16.15.** Metal HLE is therefore only
  needed to track current live patches, not to get a picture on screen.
- Fixup format is stable across two years and both architectures — no chained-fixups
  work needed anywhere yet.
- 16.3 vs 16.15 surfaces are otherwise nearly identical (392 libSystem, ~33 CF, 22
  libobjc, 27–28 AppKit symbols) — work done for 16.3 transfers wholesale.
- 16.3's arm64 slice + LC_DYLD_INFO means a future native aarch64-Linux port needs only
  the arm64 TLV/msgSend shims, not a new loader.
