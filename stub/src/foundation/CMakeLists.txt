# Foundation — NSUserDefaults suite + NS numeric helpers (Misc.m splits in a later stage)
add_library(stub-foundation OBJECT
    Misc.m
)
target_include_directories(stub-foundation PRIVATE ${STUB_SRC_DIR})
target_link_libraries(stub-foundation PRIVATE stub-flags GNUStep sdl2)
