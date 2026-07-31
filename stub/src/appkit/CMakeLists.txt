# AppKit — Cocoa UI surface over SDL2
add_library(stub-appkit OBJECT
    defines.h
    enums.h
    input_debug.h
    NSAlert.h
    NSAlert.m
    NSApplication.h
    NSApplication.m
    NSCursor.h
    NSCursor.m
    NSEvent.h
    NSEvent.m
    NSImage.h
    NSImage.m
    NSImageView.h
    NSImageView.m
    NSProgressIndicator.h
    NSProgressIndicator.m
    NSNib.h
    NSNib.m
    NSOpenGLContext.h
    NSOpenGLContext.m
    NSPasteboard.h
    NSPasteboard.m
    NSResponder.h
    NSResponder.m
    NSTextInputContext.h
    NSTextInputContext.m
    NSScreen.h
    NSScreen.m
    NSView.h
    NSView.m
    NSWindow.h
    NSWindow.m
    NSWindowController.h
    NSWindowController.m
    NSWorkspace.h
    NSWorkspace.m
)
target_include_directories(stub-appkit PRIVATE ${STUB_SRC_DIR})
target_link_libraries(stub-appkit PRIVATE stub-flags GNUStep sdl2 GL)
