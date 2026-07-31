# Foundation — NSUserDefaults suite + NS numeric helpers
add_library(stub-foundation OBJECT
    NSUserDefaultsStubs.m
    NSValueStubs.m
)
target_include_directories(stub-foundation PRIVATE ${STUB_SRC_DIR})
target_link_libraries(stub-foundation PRIVATE stub-flags GNUStep sdl2)
