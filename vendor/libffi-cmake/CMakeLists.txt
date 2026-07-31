# Hand-written CMake build of the vendored libffi (v3.5.2).
#
# Same two rules as vendor/gnustep-base-cmake (and the same reason: libffi is
# autotools-only upstream, with no CMake support at all):
#
#  1. The vendored sources under ../libffi are NOT modified.  Everything
#     `configure` generates lives frozen under ../libffi-config/<triple>/ and goes
#     on the include path in front of the vendored headers.
#  2. The source list, the defines and the version script below are transcribed
#     from a real `./configure && make` of v3.5.2 on this host (clang, Fedora 44,
#     x86_64-pc-linux-gnu).  `make` prints every command; that output is the
#     reference.  Do not "tidy" this without re-running it.
#
# Frozen from that run:
#   fficonfig.h   the HAVE_* answer sheet; every source does #include <fficonfig.h>
#                 under -DHAVE_CONFIG_H
#   ffi.h         include/ffi.h.in with the version/size substitutions applied
#   ffitarget.h   configure copies src/x86/ffitarget.h here; byte-identical to it,
#                 kept as a separate file only because ffi.h includes it by that
#                 name from the *include* directory
#   libffi.map    the linker version script: libffi.map.in run through the C
#                 preprocessor with -DX86_64 (the linemarkers stripped).  Without
#                 it libffi.so exports its internals and carries no symbol
#                 versions, so it would not be a drop-in for the system one.

if(NOT (CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64"))
    message(FATAL_ERROR
        "vendor/libffi-config only has frozen configure output for "
        "x86_64-linux-gnu, not ${CMAKE_SYSTEM_PROCESSOR}-${CMAKE_SYSTEM_NAME}. "
        "See docs/GNUSTEP-BUILD.md.")
endif()

set(FFI_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../libffi")
set(FFI_CFG "${CMAKE_CURRENT_SOURCE_DIR}/../libffi-config/x86_64-linux-gnu")

# The exact ten objects that went into libffi.so.8.2.0.  src/x86 also holds ffi.c,
# sysv.S, sysv_intel.S and win64_intel.S -- configure.host selects none of them for
# a 64-bit ELF target (they are the i386 and MSVC-assembler variants).
set(FFI_SOURCES
    src/prep_cif.c
    src/types.c
    src/raw_api.c
    src/java_raw_api.c
    src/closures.c
    src/tramp.c
    src/x86/ffi64.c
    src/x86/unix64.S
    src/x86/ffiw64.c
    src/x86/win64.S
)
list(TRANSFORM FFI_SOURCES PREPEND "${FFI_SRC}/")

# win64.S and ffiw64.c are NOT Windows-only: on x86_64 they implement the
# FFI_EFI64/FFI_WIN64 ABI variant that ffi64.c dispatches to, and the autotools
# build compiles them on Linux too.
add_library(ffi SHARED ${FFI_SOURCES})

# Both C and ASM claim `.S` in CMake's extension map, exactly like `.m` for
# C/OBJC (see vendor/CMakeLists.txt).  Pin it: the preprocessed-assembly path is
# the only one that works here -- unix64.S and win64.S both #include <fficonfig.h>
# and expand libffi's own macros.
set_source_files_properties(
    "${FFI_SRC}/src/x86/unix64.S" "${FFI_SRC}/src/x86/win64.S"
    PROPERTIES LANGUAGE ASM)

set_target_properties(ffi PROPERTIES
    OUTPUT_NAME ffi
    POSITION_INDEPENDENT_CODE ON
    # libtool-version says 8:2:0 -> libffi.so.8.2.0, soname libffi.so.8.
    VERSION 8.2.0
    SOVERSION 8
)

target_compile_definitions(ffi PRIVATE HAVE_CONFIG_H)
# -fexceptions is load-bearing, not decoration: libffi's closures have to be
# unwindable through, which is what makes NSInvocation's forwarding work at all.
target_compile_options(ffi PRIVATE -Wall $<$<COMPILE_LANGUAGE:C>:-fexceptions>)

# Frozen configure output first, then the vendored headers.
target_include_directories(ffi PRIVATE
    "${FFI_CFG}"
    "${FFI_SRC}/include"
    "${FFI_SRC}/src"
)
# Consumers need ffi.h + ffitarget.h, and those are the generated ones.
target_include_directories(ffi INTERFACE "${FFI_CFG}")

target_link_options(ffi PRIVATE "-Wl,--version-script,${FFI_CFG}/libffi.map")
set_property(TARGET ffi APPEND PROPERTY LINK_DEPENDS "${FFI_CFG}/libffi.map")
