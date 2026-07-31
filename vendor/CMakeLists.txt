# Vendored ObjC runtime + Foundation, replacing the podman-built deps/sysroot.
#
# Everything here reproduces what deps/Containerfile used to build; see
# docs/GNUSTEP-BUILD.md for the ABI rules that are NOT negotiable, in particular
# the libc++/libc++abi pairing for libobjc2.
#
# Pins (git subtree --squash, no upstream history):
#   vendor/libobjc2      libobjc2  v2.3         e877e782fb965c4e870d2ecf6aff58dddc6290ae
#   vendor/gnustep-base  libs-base base-1_29_0  c6af8ef4bf44c2d3ed36a136f1799fe263978af4
#   vendor/robin-map     robin-map v1.3.0        188c45569cc2a5dd768077c193830b51d33a5020
#   vendor/libdispatch   swift-corelibs-libdispatch swift-6.0.2-RELEASE
#                                                137b6cf3060eae87d9b367c263619c2eca5d3aac
#   vendor/libxml2       libxml2   v2.12.10     02d577715e1951cf114ca4e9adaf5605156c4f4c
#   vendor/libffi        libffi    v3.5.2       e2eda0cf72a0598b44278cc91860ea402273fa29
#   vendor/SDL2          SDL       release-2.32.10
#                                                5d249570393f7a37e037abf22cd6012a4cc56a71
#
# The full inventory, with the reason each dep is vendored (or deliberately not),
# lives in vendor/README.md.

# ---------------------------------------------------------------------------
# libobjc2 (CMake-native)
# ---------------------------------------------------------------------------
# THE ABI LANDMINE (docs/GNUSTEP-BUILD.md): -stdlib=libc++ must be in BOTH
# CMAKE_CXX_FLAGS and CMAKE_OBJCXX_FLAGS.  With only the former, arc.mm (ObjC++)
# compiles against libstdc++ headers and libobjc.so ships an unsatisfiable
# std::__throw_length_error(char const*), which breaks every later -lobjc link.
#
# These are saved/restored around add_subdirectory(libobjc2) because libobjc2 is
# the ONLY component here with C++ in it.  gnustep-base is pure C/ObjC and must
# not acquire a libc++abi DT_NEEDED it never had in the sysroot build.
set(_saved_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(_saved_OBJCXX_FLAGS "${CMAKE_OBJCXX_FLAGS}")
set(_saved_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
set(CMAKE_OBJCXX_FLAGS "${CMAKE_OBJCXX_FLAGS} -stdlib=libc++")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++ -lc++abi")

# The root CMakeLists forces -std=c++20 on every configuration for macholo's own
# C++.  libobjc2 asks for CMAKE_CXX_STANDARD 17 and the reference (Containerfile)
# build compiled it as gnu++17, so strip the override back out here only.
foreach(_cfg RELEASE DEBUG RELWITHDEBINFO MINSIZEREL)
    string(REPLACE "-std=c++20" "" CMAKE_CXX_FLAGS_${_cfg} "${CMAKE_CXX_FLAGS_${_cfg}}")
endforeach()

# Options as the Containerfile configured them: only -DTESTS=OFF was passed, so
# every other option keeps its upstream default.  Spelled out here because the
# defaults are load-bearing:
#   EMBEDDED_BLOCKS_RUNTIME=ON  -> libobjc.so exports _Block_copy itself (the
#                                  known duplicate-symbol wart vs libBlocksRuntime)
#   OLDABI_COMPAT=ON            -> legacy.c/abi_version.c/statics_loader.c in
#   ENABLE_OBJCXX=ON            -> arc.mm, objcxx_eh.cc: the ObjC<->C++ EH interop
set(TESTS OFF CACHE BOOL "libobjc2: build the runtime test suite" FORCE)
set(EMBEDDED_BLOCKS_RUNTIME ON CACHE BOOL "libobjc2: built-in blocks runtime" FORCE)
set(BUILD_STATIC_LIBOBJC OFF CACHE BOOL "libobjc2: also build libobjc.a" FORCE)
# Do not let libobjc2 discover a gnustep-config and redirect its install rules
# into a GNUstep domain; nothing installs from this tree.
set(GNUSTEP_INSTALL_TYPE "NONE" CACHE STRING "libobjc2: install layout" FORCE)

# tsl::robin_map comes from vendor/robin-map (header-only), not from the host's
# robin-map-devel and NOT from libobjc2's FetchContent fallback, which would clone
# from GitHub at configure time.  See vendor/robin-map-cmake/tsl-robin-mapConfig.cmake.
# FORCE, because find_package caches <pkg>_DIR and a stale cache entry pointing at
# /usr/share/cmake/tsl-robin-map would silently put us back on the host package.
set(tsl-robin-map_DIR "${CMAKE_CURRENT_SOURCE_DIR}/robin-map-cmake"
    CACHE PATH "vendored tsl-robin-map package config" FORCE)

# libobjc2 assumes it is the top-level project; libobjc2-fixup.cmake undoes that
# assumption from inside its own scope. See that file.
set(CMAKE_PROJECT_libobjc_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/libobjc2-fixup.cmake")

add_subdirectory(libobjc2 EXCLUDE_FROM_ALL)

# Pin the language of libobjc2's ObjC sources, for the reason gnustep-base-cmake
# already pins its own (see the comment there): both C and OBJC claim the `.m`
# extension in CMake's extension->language map -- likewise CXX and OBJCXX for `.mm`
# -- and which one wins depends on the order the languages happen to be
# (re)initialised, which is not stable across reconfigures of an existing cache.
# gnustep-base pins from inside its own CMakeLists; libobjc2 cannot, because the
# vendored file must stay byte-identical to upstream v2.3, so it is pinned here.
#
# The damage when `.m` loses to C is specific and hard to read back from the
# symptom.  Upstream applies -fobjc-runtime=gnustep-2.0 through a
# $<COMPILE_LANGUAGE:OBJC> generator expression, so the flag silently disappears and
# associate.m, properties.m and fast_paths.m compile against the legacy GCC runtime
# instead.  Their __objc_exec_class constructors then set the runtime to LegacyABI
# before libobjc2's own __objc_load constructor runs, and *every* macholo binary
# aborts before main with:
#
#     Version 2 Objective-C ABI may not be mixed with earlier versions.
#
# Observed once in the wild (2026-07-31, during the null-Metal bring-up), which is
# what prompted this pin.
set(_libobjc_objc_srcs
    NSBlocks.m associate.m blocks_runtime.m blocks_runtime_np.m fast_paths.m
    mutation.m properties.m)
list(TRANSFORM _libobjc_objc_srcs PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/libobjc2/")
set_source_files_properties(${_libobjc_objc_srcs}
    DIRECTORY libobjc2 PROPERTIES LANGUAGE OBJC)
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/libobjc2/arc.mm"
    DIRECTORY libobjc2 PROPERTIES LANGUAGE OBJCXX)
unset(_libobjc_objc_srcs)

# objc/runtime.h does #include "objc-config.h", which libobjc2 generates into its
# own binary dir; consumers need that dir on the include path too.
target_include_directories(objc INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/libobjc2/objc>)

set(CMAKE_CXX_FLAGS "${_saved_CXX_FLAGS}")
set(CMAKE_OBJCXX_FLAGS "${_saved_OBJCXX_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "${_saved_SHARED_LINKER_FLAGS}")

# ---------------------------------------------------------------------------
# libdispatch (CMake-native)
# ---------------------------------------------------------------------------
# swift-corelibs-libdispatch at the tag matching Fedora 44's libdispatch-6.0.2.
# gnustep-base uses it for real (7 binds in the boot trace: the run-loop
# integration and NSOperation), and it is where <Block.h> comes from, so it must
# come BEFORE gnustep-base-cmake.
#
# Placed after the libc++ flag block above is restored, on purpose: the reference
# libdispatch.so needs only libBlocksRuntime.so + libc.so.6, and giving it a
# libc++ DT_NEEDED would be the same class of mistake as the arc.mm landmine.
#
# Upstream option handling has three traps, all of them cache-level:
#
#  * include(CTest) does option(BUILD_TESTING ... ON), so tests/ would be
#    configured and built. Pre-seed the cache entry so the option() is a no-op.
#  * HAVE_OBJC comes from check_include_files(objc/objc-internal.h). If a host
#    ever grew that header, src/CMakeLists.txt would add data.m, object.m and
#    swift/DispatchStubs.cc -- and libdispatch's project() declares LANGUAGES C
#    CXX only, so the `.m` files would land on whichever language currently owns
#    the extension (see the .m pin below) with none of the ObjC flags. Fedora's
#    own libdispatch is built without it (zero OS_dispatch_* symbols in
#    /lib64/libdispatch.so), so pin it off and keep the build host-independent.
#  * ENABLE_SWIFT would enable_language(Swift) globally.
set(BUILD_TESTING OFF CACHE BOOL "libdispatch: build the test suite" FORCE)
set(ENABLE_SWIFT OFF CACHE BOOL "libdispatch: Swift overlay" FORCE)
set(ENABLE_DTRACE OFF CACHE BOOL "libdispatch: dtrace probes" FORCE)
set(INSTALL_PRIVATE_HEADERS OFF CACHE BOOL "libdispatch: install private headers" FORCE)
set(HAVE_OBJC FALSE CACHE INTERNAL "libdispatch: pinned off, see vendor/CMakeLists.txt")

# THE SECOND C++-RUNTIME LANDMINE, same family as libobjc2's above. src/block.cpp
# is the one C++ file in libdispatch and it uses no part of the C++ runtime -- the
# reference /lib64/libdispatch.so needs only libBlocksRuntime.so and libc.so.6.
# But a CMake target holding *any* CXX source gets CMAKE_CXX_IMPLICIT_LINK_LIBRARIES
# (`stdc++;m` for clang here) appended to its link line even when the link language
# is C, so libdispatch.so acquires a DT_NEEDED on libstdc++.so.6 -- a second
# Itanium C++ ABI in a process whose ObjC<->C++ exception interop runs through
# libc++abi. That is exactly the pairing docs/GNUSTEP-BUILD.md says not to create
# (it is why -lsupc++ was dropped in 2026-07). Empty the list for this subtree only.
set(_saved_CXX_IMPLICIT_LINK_LIBRARIES "${CMAKE_CXX_IMPLICIT_LINK_LIBRARIES}")
set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "")

add_subdirectory(libdispatch EXCLUDE_FROM_ALL)

set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "${_saved_CXX_IMPLICIT_LINK_LIBRARIES}")
unset(_saved_CXX_IMPLICIT_LINK_LIBRARIES)

# libdispatch's own CMakeLists does `set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
# ${CMAKE_BINARY_DIR})` -- a Windows PATH workaround that means "my own build
# dir" but resolves to macholo's TOP-level build dir under add_subdirectory.
# Same family of bug as libobjc2's CMAKE_SOURCE_DIR use. Put the two libraries
# back where every other vendored target puts them.
set_target_properties(dispatch BlocksRuntime PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/libdispatch"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/libdispatch")

# block.cpp is the only C++ in libdispatch and it touches no C++ runtime -- the
# reference libdispatch.so links neither libstdc++ nor libc++. Link with the C
# driver so the C++ stdlib is never pulled in.
set_target_properties(dispatch PROPERTIES LINKER_LANGUAGE C)

# DispatchCompilerWarnings.cmake turns on -Werror -Wall -Wextra -Wsign-conversion
# and friends, tuned for the clang of 2024. clang 22 fails the 6.0.2 sources on at
# least -Wcast-function-type-mismatch (apply.c, data.c cast a dispatch_function_t
# to a differently-shaped applier on purpose) and -Wsign-conversion
# (event/event_epoll.c). Blanket-demote rather than chase the list: this is
# upstream code we do not maintain, and it is the same call deps/Containerfile's
# RELAX_CFLAGS made for gnustep-base. Warnings still print.
target_compile_options(dispatch PRIVATE -Wno-error)
target_compile_options(BlocksRuntime PRIVATE -Wno-error)

# ---------------------------------------------------------------------------
# libxml2 (CMake-native)
# ---------------------------------------------------------------------------
# Pinned to v2.12.10 == Fedora 44's libxml2-2.12.10-6.fc44, deliberately: the only
# gnustep-base patch in this tree (Source/Additions/GSXML.m, the xmlEntity.checked
# removal) is a >= 2.12 fixup, and a newer libxml2 would be behaviour drift on the
# plist/XML path for no gain.
#
# Only four defaults are changed, all of them things nothing here consumes; every
# other LIBXML2_WITH_* keeps upstream's default so the parser behaves as the RPM
# does.  PROGRAMS/TESTS are build-time only; LZMA and ZLIB are the two options that
# would each add a DT_NEEDED (liblzma, libz) for compressed-input support that
# neither GSXML nor NSXMLParser ever asks for.  gnustep-base still links -lz on its
# own account.
set(LIBXML2_WITH_PYTHON   OFF CACHE BOOL "libxml2: Python bindings" FORCE)
set(LIBXML2_WITH_TESTS    OFF CACHE BOOL "libxml2: test suite" FORCE)
set(LIBXML2_WITH_PROGRAMS OFF CACHE BOOL "libxml2: xmllint/xmlcatalog" FORCE)
set(LIBXML2_WITH_LZMA     OFF CACHE BOOL "libxml2: liblzma input" FORCE)
set(LIBXML2_WITH_ZLIB     OFF CACHE BOOL "libxml2: libz input" FORCE)

add_subdirectory(libxml2 EXCLUDE_FROM_ALL)

# ---------------------------------------------------------------------------
# libffi (hand-written CMake; upstream is autotools-only)
# ---------------------------------------------------------------------------
add_subdirectory(libffi-cmake)

# ---------------------------------------------------------------------------
# gnustep-base (hand-written CMake over the vendored sources)
# ---------------------------------------------------------------------------
add_subdirectory(gnustep-base-cmake)

# ---------------------------------------------------------------------------
# SDL2 (CMake-native)
# ---------------------------------------------------------------------------
# stub/ implements AppKit on SDL2; this is the window/input/GL-context layer the
# game actually runs on, so it is the one vendored dep that is not a GNUstep
# dependency at all.  Opt out with -DMACHOLO_SYSTEM_SDL=ON (see stub/CMakeLists.txt).
#
# Pinned to release-2.32.10, the newest release of the 2.32 line.  Note that
# Fedora 44 has no SDL2 package at all: `dnf` gives you sdl2-compat 2.32.70, which
# is an SDL2 API shim *on top of SDL3*.  So there is no system SDL2 release to
# match exactly, and vendoring here swaps a two-layer shim for the real thing.
#
# Build only what the stub links: the shared library.  SDL2's own test suite,
# static library and install rules are all off.
set(SDL_SHARED ON CACHE BOOL "SDL2: build libSDL2.so" FORCE)
set(SDL_STATIC OFF CACHE BOOL "SDL2: build libSDL2.a" FORCE)
set(SDL_TEST OFF CACHE BOOL "SDL2: build the test harness library" FORCE)
set(SDL2_DISABLE_INSTALL ON CACHE BOOL "SDL2: install rules" FORCE)
set(SDL2_DISABLE_UNINSTALL ON CACHE BOOL "SDL2: uninstall target" FORCE)

add_subdirectory(SDL2 EXCLUDE_FROM_ALL)

# ---------------------------------------------------------------------------
# CoreFoundation headers for mini-CF (headers only, no corebase code)
# ---------------------------------------------------------------------------
add_library(corebase-headers INTERFACE)
target_include_directories(corebase-headers INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/corebase-headers")

# ---------------------------------------------------------------------------
# The aggregate the consumer links against.
# ---------------------------------------------------------------------------
add_library(gnustep-vendored INTERFACE)
target_link_libraries(gnustep-vendored INTERFACE gnustep-base corebase-headers)
