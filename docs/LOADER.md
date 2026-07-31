# macholo loader — real vs faked ledger

Running reference for what the loader/runtime genuinely implements vs what it fakes,
stubs, or doesn't do yet. Purpose: per-game bring-up (WC3, future titles) should read
this instead of re-deriving loader state from `git log`/source each time. Cross-refs
`docs/PLAN.md` (roadmap/phases) and `docs/AUDIT-*.md` (per-binary surfaces); this file
doesn't duplicate those, it's the "what actually runs" ledger.

Sources: `src/loader.{cpp,hpp}`, `src/macho.{cpp,hpp}`, `src/lib.cpp`, `src/objc.cpp`,
`src/tlv.cpp`, `stub/src/`, and `git log` on `master`/`wc3-bringup` as of 2026-07-31
(18 commits into WC3 bring-up, HEAD `fa69e80`).

## 1. What's real

Loading / mapping:
- **FAT/universal slicing**, host-arch select: `macho::find_slice` (`src/macho.hpp`
  `Slice`, used in `loader::Image::load`, `src/loader.cpp:145`). Thin files return
  `{0, size}`.
- **Per-segment mapping with real init/max protections + zerofill**:
  `loader::map_segments` (`src/loader.cpp:108-131`) maps one `mmap` per segment at
  `vmaddr+slide`, temporarily `| PROT_WRITE` for fixups; `loader::Image::protect()`
  (`src/loader.cpp:224-235`) drops each segment to `final_prot()`
  (`src/loader.cpp:45-52`), which folds `initprot` and clears `PROT_WRITE` for
  `SG_READ_ONLY` segments (`__DATA_CONST`'s "declared rw-, ends up r/o" trick).
  Zerofill sections (`__bss`/`__common`, thread-local zerofill) are explicitly
  `memset` to 0 even when they start before the end of the file-backed part of their
  segment (`src/loader.cpp:204-211`) — a real bug class found on 16.3 (`__DATA`
  zerofill tail past the file slice).
- **`__DATA_CONST`/section search across all segments**: `Image::find_section`
  (`src/loader.hpp:23-34`) walks every segment's section list rather than assuming a
  fixed segment, needed because modern binaries split `__mod_init_func`,
  `__objc_classlist`, `__cfstring` etc. into `__DATA_CONST`.
- **Classic `LC_DYLD_INFO_ONLY` bind opcodes**: full opcode interpreter in
  `src/macho.cpp` (`BIND_OPCODE_*` enum ~line 237, dispatch ~line 899-970) driving
  `object.bind`/`object.bind_lazy`/`object.bind_weak`; consumed in
  `loader::Image::bind()` (`src/loader.cpp:237-264`).
- **VM-address-laid-out `MAP_SHARED` backing file** so `mach_vm_remap` can alias:
  `make_backing()` (`src/loader.cpp:81-98`) builds a `memfd` indexed by
  `vmaddr - vm_base` (not file offset), and `map_segments` maps every segment
  `MAP_SHARED` from it. This is what lets `lib::stub::mach_vm_remap`
  (`src/lib.cpp:121-143`) hand out a second, real aliasing mapping of the same pages
  — needed for WC3's `__INIT0` packer, which decrypts `__TEXT` through a remapped
  writable alias of itself, spanning multiple segments in one call. Falls back to a
  single flat RWX overlay (`src/loader.cpp:176-190`) if the layout doesn't fit the
  per-segment assumptions (`layout_is_ordinary`, `src/loader.cpp:56-68`) — in that
  fallback, `mach_vm_remap` is unavailable (`backing_fd == -1`) and `protect()` is a
  no-op.

TLV:
- **Real `pthread_setspecific`-backed thread-locals**: `tlv::fixup_image`
  (`src/tlv.cpp:60-77`) patches each `tlv_descriptor_t` in `__thread_vars` to point at
  `tlv_thunk`; `tlv_allocate_for_key` (`src/tlv.cpp:19-44`) allocates a fresh
  block per thread, copies the `__thread_data` template into it, publishes it via
  `pthread_setspecific` *before* running `__thread_init` pointers (so a re-entrant
  initializer sees its own block, not a second competing one), then runs them. This
  was previously broken (writes never stuck — see `c04ff5e` on master); now genuinely
  keyed per-thread through pthread TSD.

Darwin thread/TSD ABI:
- **`%gs` TSD emulation via `arch_prctl`**: `stub/src/system/stub_pthread.c` (~line
  17-55) gives each thread a zeroed TSD block and calls
  `syscall(SYS_arch_prctl, ARCH_SET_GS, tsd)` so inlined `mov %gs:N` loads (pthread
  self, mach thread port) that Darwin's libc/libpthread headers emit resolve to real
  memory instead of Linux's zero `%gs` base.

ObjC:
- **Modern objc entry points**: `objc_alloc`/`objc_alloc_init` come from GNUstep
  libobjc2 v2.3 directly; the Apple-only `objc_opt_*` family
  (`objc_opt_class`/`objc_opt_isKindOfClass`/`objc_opt_new`/`objc_opt_self`) is
  hand-implemented in `stub/src/system/stub_objc.c` (~line 7-79) on top of real
  libobjc2 calls (`class_getSuperclass`, etc). `__objc_personality_v0` and
  `___chkstk_darwin` are real trampolines in `stub/src/system/stub_objc.S`.
- **`__objc_classlist` AND `__objc_catlist` (categories)**: `objc::raw::Converter`
  (`src/objc.cpp`) converts both — `convert_class_list` (line 301) and
  `convert_category_list`/`convert_category` (lines 311-315, 543-557). Categories
  attach via `class_replaceMethod` (category override semantics, not
  `class_addMethod`) since libobjc2 has no foreign-metadata category-registration
  entry point of its own. Added this bring-up (`4414fc8`).
- **Apple-semantics `object_{get,set}InstanceVariable`**:
  `stub/src/system/stub_objc.c` (~line 27-46), `$darwin$`-prefixed override, papering
  over a real semantic difference vs GNUstep libobjc2's own version (writes the ivar's
  *address* out, matching Apple's contract).

dyld surface:
- **Real `dladdr`/`_dyld_get_image_*`/`_NSGetMachExecuteHeader`**: `src/lib.cpp`
  `lib::stub::dladdr` (line 74-87) answers honestly for the main image's address
  range (`image_range`, line 58-72) — needed because WC3's `__INIT0` integrity code
  does `dladdr(own_code)->dli_fbase` and walks the mach header it finds.
  `dyld_image_count`/`dyld_get_image_header`/`dyld_get_image_name`/
  `dyld_get_image_vmaddr_slide` all answer for index 0 (the one real Mach-O image;
  everything else in the process is ELF). `_NSGetMachExecuteHeader` returns
  `Image::main->address` directly.
- **Working `dyld_stub_binder` with genuine lazy binding**: `macho::Binding` records
  each lazy entry's bind-opcode stream offset (`stream_offset`, `src/macho.hpp:117`,
  set by the bind-opcode parser). `lib::stub::dyld_stub_binder` (`src/lib.cpp:191-234`)
  is a hand-written, register-preserving `naked` trampoline (mirrors dyld's real
  `dyld_stub_binder.s`) that calls `macholo_fast_stub_entry` (line 172-185), which
  looks up the binding by `stream_offset`, resolves it via `lib::resolve_import`,
  patches the lazy pointer in place, and returns the resolved target for the tail
  jump. This matters because WC3's `__INIT0` pass resets `la_symbol_ptr` slots back to
  their original `__stub_helper` entries (to shake off IAT-style hooks) and then
  relies on real lazy binding actually working.

Darwin comm page:
- **Mapped at `0x7fffffe00000` with `mach_absolute_time` fast-path**:
  `map_comm_page()` (`src/loader.cpp:307-327`), called from `Image::run()` before
  `GSInitializeProcess`. Fills the "approximate time supported" byte at `+0x88` so
  the fast arm is taken, and keeps the `u64` at `+0x80` advancing from
  `CLOCK_MONOTONIC` via a detached updater thread every 250us
  (`comm_page_updater`, line 298-305) — needed because `mach_absolute_time()` is
  inlined by the compiler as a direct load of this page, not a callable import, so no
  stub can intercept it. The rdtsc-path fields (`tsc_base`/`scale`/`shift`/
  `ns_base`/`generation`) are also filled with sane values in case that arm is ever
  taken.

## 2. What's faked/stubbed

- **ObjC runtime**: GNUstep libobjc2, vendored — not an own runtime. `src/objc.cpp`'s
  `Converter` translates Apple ABI metadata (`class_t`/`category_t`/`protocol_t`/
  `class_ro_t` as laid out by Apple's compiler) into libobjc2 registration calls
  (`objc_allocateClassPair`, `class_addMethod`, `class_addIvar`, ...). This is a
  metadata *converter*, not a reimplementation of either ABI.
- **gnustep-base/corebase** stand in for Foundation/CF. Includes the known **CFURL/
  CFBundle inconsistency**, papered over via the `$darwin$` symbol-prefix override
  mechanism in `lib::resolve` (`src/lib.cpp:255-309`) — GNUstep and Apple disagree on
  some CFURL/CFBundle semantics, and `$darwin$`-prefixed symbols let a stub override
  win over the real GNUstep implementation without touching GNUstep itself. See also
  `stub/src/misc/CFURL.m`.
- **Hand-written AppKit/Carbon/CG/Security stubs**: `stub/src/appkit/*`
  (NSWindow, NSView, NSAlert, NSWorkspace, SBApplication, NSWindowController, ...),
  `stub/src/system/stub_carbon.m` (text-input sources, key state, FSRef),
  `stub/src/coregraphics/CG.{h,m}` (geometry, display config, event taps, bitmap
  contexts), `stub/src/system/stub_securetransport.c` (SecureTransport/trust
  evaluation/code signing). These implement just enough of each framework's surface
  for the games' actual call patterns, not the real frameworks.
- **Frameworks bundled with games are stubbed, NOT loaded**: e.g. RPatch/mvg (WC3/
  League patcher + telemetry dylibs) are satisfied by stubbing their ~18 imports
  ("nothing to do" / disconnected telemetry) rather than by loading the bundled
  `.dylib` — see PLAN.md phase 2 note ("multi-image loader not needed"). Same
  treatment applies to any other bundled framework a game dlopens: `dlopen`
  (`src/lib.cpp` `lib::stub::dlopen`, line 155-160) only recognizes `libc*` by name
  and returns a sentinel; everything else returns `NULL`.
- **Battle.net ClientSdk (WC3, new this bring-up)**:
  `stub/src/system/stub_misc.c` (~line 214-221) — `ClientSdk_Init`,
  `ClientSdk_CreateInstance`, `ClientSdk_GetInterface` all return `NULL`;
  `ClientSdk_DeleteInstance`/`ClientSdk_Shutdown` are no-ops. This is a deliberate
  "return unavailable" stub for the offline path, not real Battle.net client logic —
  and per Phase 2 below, `NULL` alone isn't suffient; see the open wall.
- **Mach VM/thread-inspection surface** (`stub/src/system/stub_mach_vm.c`): answers
  honestly where cheap (`vm_copy` does a real `memcpy`), otherwise fails cleanly
  (`mach_vm_region` → `KERN_INVALID_ADDRESS`, `task_threads` → empty list) rather than
  emulating real task/thread introspection — this is anti-tamper/crash-reporter
  territory that has no honest Linux answer.

## 3. What's NOT done yet (honest gaps)

- **Single-image only**: `main.cpp` loads exactly one `loader::Image` (`argv[1]`);
  there is no real `dlopen` of a second Mach-O image. `lib::stub::dlopen` only
  whitelists `libc` by substring match and returns a sentinel value, never an actual
  second loaded image (`src/lib.cpp:155-160`). Blocks: any game that calls into
  a genuinely-needed bundled dylib rather than one whose imports can be
  stubbed/no-op'd (RPatch/mvg get away with it; a game requiring real bundled-dylib
  *behavior* would not).
- **No `LC_DYLD_CHAINED_FIXUPS`**: the load command is recognized just enough to
  keep the parser from choking (`impl_command(LC_DYLD_CHAINED_FIXUPS,
  linkedit_data_command)`, `src/macho.cpp:639`, mapped to the generic
  linkedit-data-command shape) but its contents (the chained-fixups header,
  starts-in-segment tables, chained pointer format) are never walked or applied —
  only classic `LC_DYLD_INFO_ONLY` bind opcodes are interpreted. Blocks: any binary
  linked with modern chained fixups instead of classic bind (none of 14.8/16.3/16.15/
  WC3 1.31 need this so far, per AUDIT docs, but a newer Xcode/linker target would).
- **Metal HLE deferred**: see `docs/PLAN.md` phase 4. `MTLCreateSystemDefaultDevice`
  and friends currently resolve to NULL/absent, which happens to make 16.3 fall back
  to its GL renderer on its own. Blocks: 16.15 (Metal-only, GL renderer removed) and
  any future Metal-only title.
- **Input fidelity partial**: per `docs/PLAN.md` phase-1 remaining loose ends
  (`349452a`/`a5f0ef0` era) — event tracing is gated/partial, some AppKit input-path
  stub classes (`NSProgressIndicator`, `NSImageView`) are minimal placeholders rather
  than functional. Low priority so far since replays don't need real input.
- **WC3 Battle.net ClientSdk wall (current, in progress)**: WC3 SIGSEGVs inside its
  offline-handling path because the game's `if (!handle) goto offline;` offline
  branch itself dereferences state through `handle` that a bare `NULL` doesn't
  provide — the stub needs to hand back *some* inert non-NULL ClientSdk-shaped
  object, not just fail the init call. Being probed on a separate experimental branch
  (`wc3-sdk-probe`), not `wc3-bringup` — see that branch for findings; nothing from
  it is merged here.
- **Multi-init / module-term functions**: `Image::init()` runs `__mod_init_func`
  pointers (`src/loader.cpp:266-271`) but there is no handling of
  `__mod_term_func`/`atexit`-driven teardown — `lib::stub::atexit`/`cxa_atexit`
  (`src/lib.cpp:242-252`) are both explicit no-ops ("need proper exit support").
  Blocks: nothing observed yet (games don't rely on clean exit), but flagged as a
  known gap.
