project(LANGUAGES C ASM OBJC)

add_subdirectory(gnustep)

find_package(SDL2 REQUIRED)
add_library(sdl2 INTERFACE)
target_include_directories(sdl2 INTERFACE ${SDL2_INCLUDE_DIRS})
target_link_libraries(sdl2 INTERFACE ${SDL2_LIBRARIES})

add_library(stub SHARED
    # AppKit
    src/appkit/NSAlert.h
    src/appkit/NSAlert.m
    src/appkit/NSApplication.h
    src/appkit/NSApplication.m
    src/appkit/NSCursor.h
    src/appkit/NSCursor.m
    src/appkit/NSEvent.h
    src/appkit/NSEvent.m
    src/appkit/NSImage.h
    src/appkit/NSImage.m
    src/appkit/NSImageView.h
    src/appkit/NSImageView.m
    src/appkit/NSProgressIndicator.h
    src/appkit/NSProgressIndicator.m
    src/appkit/NSNib.h
    src/appkit/NSNib.m
    src/appkit/NSOpenGLContext.h
    src/appkit/NSOpenGLContext.m
    src/appkit/NSResponder.h
    src/appkit/NSResponder.m
    src/appkit/NSTextInputContext.h
    src/appkit/NSTextInputContext.m
    src/appkit/NSScreen.h
    src/appkit/NSScreen.m
    src/appkit/NSView.h
    src/appkit/NSView.m
    src/appkit/NSWindow.h
    src/appkit/NSWindow.m

    # CoreGraphics
    src/coregraphics/CG.h
    src/coregraphics/CG.m
    
    # Misc IOKit and CoreSecurity
    src/misc/Misc.m

    # libSystem
    src/system/stub_asl.c
    src/system/stub_audio.c
    src/system/stub_dispatch.c
    src/system/stub_fs.c
    src/system/stub_io.c
    src/system/stub_kernel.c
    src/system/stub_kernel_kqueue.c
    src/system/stub_math.c
    src/system/stub_misc.c
    src/system/stub_nvcg.c
    src/system/stub_objc.S
    src/system/stub_objc.c
    src/system/stub_process.c
    src/system/stub_pthread.c
    src/system/stub_resolv.c
    src/system/stub_rune.c
    src/system/stub_socket.c
    src/system/stub_str.c
    src/system/stub_sys.c
    src/system/stub_video.c
    src/system/stub_xpc.c
)
target_compile_options(stub PRIVATE -Wno-nullability-completeness -Wno-c23-extensions)
target_include_directories(stub PRIVATE src/)
target_link_libraries(stub PUBLIC GNUStep sdl2 GL)
