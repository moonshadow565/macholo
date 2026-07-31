# Metal — deliberately its own module: this is the seam where 16.15 Metal HLE will grow.
add_library(stub-metal OBJECT
    MetalStubs.c
)
target_include_directories(stub-metal PRIVATE ${STUB_SRC_DIR})
target_link_libraries(stub-metal PRIVATE stub-flags GNUStep sdl2)
