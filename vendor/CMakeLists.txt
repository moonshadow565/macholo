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
# gnustep-base (hand-written CMake over the vendored sources)
# ---------------------------------------------------------------------------
add_subdirectory(gnustep-base-cmake)

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
