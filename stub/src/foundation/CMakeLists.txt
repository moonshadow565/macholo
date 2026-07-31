# Foundation — NSUserDefaults suite + NS numeric helpers, plus leftover Sec*/IOHID/SC/Metal
# stubs that fit neither the CF nor NS surface (see docs/REFACTOR.md Stage 2b report)
add_library(stub-foundation OBJECT
    IOHIDStubs.c
    MetalStubs.c
    NSUserDefaultsStubs.m
    NSValueStubs.m
    SCDynamicStoreStubs.c
    SecurityStubs.m
)
target_include_directories(stub-foundation PRIVATE ${STUB_SRC_DIR})
target_link_libraries(stub-foundation PRIVATE stub-flags GNUStep sdl2)
