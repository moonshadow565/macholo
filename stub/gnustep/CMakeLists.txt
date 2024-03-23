add_library(GNUStep INTERFACE)

set(GNUSTEP_DIR "/usr/GNUstep" CACHE STRING "Directory for GNUstep")

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
    -lgnustep-corebase
)
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
