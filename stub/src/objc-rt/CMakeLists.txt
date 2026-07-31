# objc runtime — objc entry-point shims (portable C half; thunks live in arch-x86_64)
add_library(stub-objc-rt OBJECT
    stub_objc.c
)
target_include_directories(stub-objc-rt PRIVATE ${STUB_SRC_DIR})
target_link_libraries(stub-objc-rt PRIVATE stub-flags GNUStep sdl2)
