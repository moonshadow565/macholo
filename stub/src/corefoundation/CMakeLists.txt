# CoreFoundation — the CF surface (track-2 seam)
add_library(stub-corefoundation OBJECT
    CFBase.m
    CFBundle.m
    CFNotificationCenter.c
    CFURL.m
    minicf.h
)
target_include_directories(stub-corefoundation PRIVATE ${STUB_SRC_DIR})
target_link_libraries(stub-corefoundation PRIVATE stub-flags GNUStep sdl2)
