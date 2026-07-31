# Cg — NVIDIA Cg shader runtime (cgCreateContext & co), unrelated to CoreGraphics
add_library(stub-cg OBJECT
    stub_nvcg.c
)
target_include_directories(stub-cg PRIVATE ${STUB_SRC_DIR})
target_link_libraries(stub-cg PRIVATE stub-flags GNUStep sdl2)
