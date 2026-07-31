# CoreGraphics — CGDisplay/CGL surface
add_library(stub-coregraphics OBJECT
    CG.h
    CG.m
)
target_include_directories(stub-coregraphics PRIVATE ${STUB_SRC_DIR})
target_link_libraries(stub-coregraphics PRIVATE stub-flags GNUStep sdl2 GL)
