# The GNUstep dependency surface every stub module and the loader compile against.
#
# Two providers:
#   * default          -- the vendored source builds in vendor/ (libobjc2 + gnustep-base
#                         + corebase's CoreFoundation headers). No sysroot, no podman.
#   * -DGNUSTEP_DIR=X  -- escape hatch to a prebuilt sysroot laid out the GNUstep way
#                         (X/Local/... and X/System/...), e.g. deps/sysroot/usr/GNUstep.
# GNUSTEP_DIR is declared in the top-level CMakeLists (it decides whether vendor/ is
# added at all).

add_library(GNUStep INTERFACE)

if(GNUSTEP_DIR)
    message(STATUS "GNUstep deps: prebuilt sysroot at ${GNUSTEP_DIR}")
    target_include_directories(GNUStep INTERFACE
        ${GNUSTEP_DIR}/Local/Library/Headers
        ${GNUSTEP_DIR}/System/Library/Headers
    )
    target_link_directories(GNUStep INTERFACE
        ${GNUSTEP_DIR}/Local/Library/Libraries
        ${GNUSTEP_DIR}/System/Library/Libraries
    )
    target_link_libraries(GNUStep INTERFACE
        -lm
        -lpthread
        -lobjc
        -lgnustep-base
    )
else()
    message(STATUS "GNUstep deps: vendored source build (vendor/)")
    target_link_libraries(GNUStep INTERFACE -lm -lpthread gnustep-vendored)
endif()

target_compile_options(GNUStep INTERFACE
    -DGNUSTEP
    -DGNUSTEP_BASE_LIBRARY=1
    -DGNUSTEP_RUNTIME=1
    -D_NONFRAGILE_ABI=1
    -DGNUSTEP_BASE_LIBRARY=1
    -fobjc-runtime=gnustep-2.1
    -fblocks
    -fconstant-string-class=NSConstantString
)
