# libSystem — the mach/BSD/libc surface
add_library(stub-libsystem OBJECT
    stub_asl.c
    stub_compat.c
    stub_dispatch.c
    stub_errno.c
    stub_signal.c
    stub_fs.c
    stub_io.c
    stub_kernel.c
    stub_kernel_kqueue.c
    stub_locale.c
    stub_mach_vm.c
    stub_malloc.c
    stub_math.c
    stub_process.c
    stub_pthread.c
    stub_resolv.c
    stub_rune.c
    stub_socket.c
    stub_str.c
    stub_sys.c
    stub_xpc.c
)
target_include_directories(stub-libsystem PRIVATE ${STUB_SRC_DIR})
target_link_libraries(stub-libsystem PRIVATE stub-flags GNUStep sdl2)
