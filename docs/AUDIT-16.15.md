# Binary audit — League of Legends 16.15 (2026-07-30)

Target: `Game/16-5/LeagueofLegends.app/Contents/MacOS/LeagueofLegends`
(despite the directory name, everything in `Game/16-5` is **16.15.8008073** —
Info.plist + all three `*-metadata.json` agree; the `16.5-*.manifest` next to it is the
odd one out). Analyzed with llvm-otool/llvm-objdump 22. Raw symbol lists:
[surface-16.15/](surface-16.15/).

## Headline facts

- **FAT binary** (`cafebabe`), 2 archs: x86_64 + arm64. macholo predates this → needs
  FAT slicing (parser already exists in customlolskins-manager).
- **Both slices still use `LC_DYLD_INFO_ONLY`** — classic dyld opcodes, no chained
  fixups, even at SDK 26 / minos 10.15 (platform macOS). Existing rebase/bind/export
  parser remains valid. The port really is just FAT + section-search fixes.
- `LC_MAIN` entry, classic `__la_symbol_ptr` + `__stub_helper` lazy stubs, `__mod_init_func`
  (not `__init_offsets`), `__objc_msgrefs` gone, no `__objc_catlist`.
- **New segment `__DATA_CONST`** holds `__got`, `__mod_init_func`, `__const`,
  `__cfstring`, `__objc_classlist`, `__objc_protolist`, `__objc_imageinfo`.
  `loader::Image::find_data_section()` only searches `__DATA` → initializers and objc
  registration would silently no-op. Must search all segments (and a correct
  per-segment mapper must make `__DATA_CONST` writable during fixups).

## Linked dylibs (29)

3 bundled (`@rpath`): RiotGamesApi.framework (+ libdiscord_partner_sdk.dylib,
libvivoxsdk.dylib), RPatch.framework, mvg.framework. macholo is single-image today and
never loads dependents; 12 lazy binds land in RPatch, 6 in mvg → **multi-image loading is
a hard requirement**, not just warnings.

26 system dylibs, of which **Metal.framework and CoreGraphics.framework are
LC_LOAD_WEAK_DYLIB**. OpenGL.framework is linked but has **zero** bound symbols (dead
dependency).

## Import surface (x86_64, non-lazy + lazy, unique symbols)

| Source | Count | Assessment |
|---|---|---|
| libSystem | 392 | mostly covered by existing `stub_*`; only 5 `dispatch_*` (semaphore/once/data), zero xpc/kevent |
| libc++ | 231 | self-solved: link host libc++ (already done) |
| CoreFoundation | 34 | 29 C functions + 5 bridged NS classes — hand-writable mini-CF |
| AppKit | 27 | the class set the existing stubs already implement + NSPasteboard/NSTrackingArea/NSWorkspace etc. |
| libobjc | 22 | **modern entry points**: `objc_alloc`, `objc_alloc_init`, `objc_opt_{class,isKindOfClass,new,self}`, `objc_msgSendSuper2`, ARC (`objc_retain/release/autorelease*`), `__objc_personality_v0`. Old `*_fixup` variants in `stub_objc.S` no longer referenced |
| CoreGraphics | 21 | CGDisplay*/event-source utilities (weak dylib) |
| RPatch / mvg | 12 / 6 | bundled — resolved by multi-image loading |
| Foundation | 11 | shallow |
| AudioToolbox/AudioUnit/CoreAudio | 18 | audio path (ak.cpp Wwise sink → ALSA covers the sink side) |
| Security / IOKit / CV / SC / CoreServices / libresolv / QuartzCore | ~22 | stub-tier |
| Metal | 8 | `MTLCreateSystemDefaultDevice` + 7 weak MTL descriptor class imports |

Total non-libc/libc++ surface: **~135 symbols + 29 objc classes.** Confirms the core
thesis: games barely touch the OS.

## ObjC classes imported (29)

Foundation basics (NSArray, NSString, NSDate, NSURL, NSBundle, NSFileManager, NSThread,
NSTask, NSProcessInfo, NSAutoreleasePool, NSAttributedString, …), the AppKit windowing
set (NSApplication, NSWindow, NSView, NSEvent, NSCursor, NSScreen, NSImage,
NSBitmapImageRep, NSAlert, NSNib, NSResponder, NSPasteboard, NSTrackingArea,
NSWorkspace), and the Metal set: **CAMetalLayer + 7 MTL descriptor classes**.

## Renderer: Metal-only

No OpenGL symbols, no NSOpenGLContext/NSOpenGLPixelFormat class imports, no Cg — the
GL-era path (which macholo's NSOpenGLContext + stub_nvcg stubs served) is gone from this
build. Rendering = `MTLCreateSystemDefaultDevice` → CAMetalLayer + descriptor classes;
device/queue/encoder usage flows through msgSend (invisible statically). Metal + the MTL
class imports are weak → resolving them to NULL should yield a graceful
"Failed to init renderer" (strings confirm that log line), which suffices for headless
replay but not for a picture. See PLAN.md phase 2.

## Concrete breakage list for macholo (vs the 16.5-era client it last ran)

1. FAT header (`cafebabe`) — slice selection needed before `MH_MAGIC_64` assert.
2. `__DATA_CONST` — section search must cover all segments.
3. Modern objc entry points (`objc_alloc`, `objc_opt_*`, `objc_msgSendSuper2` direct).
4. Bundled-framework dylib dependencies (multi-image loader).
5. Metal-only renderer (weak-linked; NULL-resolvable for headless).

TLV machinery (`__thread_vars`/`__thread_data`/`__thread_bss`) unchanged — existing
tlv.cpp approach still applies. `crashpad_info` section present; crashpad/sentry remains
stub-and-ignore territory.
