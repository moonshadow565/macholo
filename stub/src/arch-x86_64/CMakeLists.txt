# x86_64 arch quarantine — anything naming a register, an ABI struct-return
# convention, or written in .S belongs here (an arm64 port adds arch-aarch64/).
add_library(stub-arch-x86_64 OBJECT
    setjmp.S
    sincos_stret.c
    stub_objc.S
)
target_include_directories(stub-arch-x86_64 PRIVATE ${STUB_SRC_DIR})
target_link_libraries(stub-arch-x86_64 PRIVATE stub-flags GNUStep sdl2)
