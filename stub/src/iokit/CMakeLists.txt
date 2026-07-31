# IOKit — registry/service surface
add_library(stub-iokit OBJECT
    stub_iokit.c
)
target_include_directories(stub-iokit PRIVATE ${STUB_SRC_DIR})
target_link_libraries(stub-iokit PRIVATE stub-flags GNUStep sdl2)
