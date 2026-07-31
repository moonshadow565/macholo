# ScriptingBridge — SBApplication (not AppKit)
add_library(stub-scriptingbridge OBJECT
    SBApplication.h
    SBApplication.m
)
# ../appkit for SBApplication.h's "defines.h" (contents are not touched at this stage)
target_include_directories(stub-scriptingbridge PRIVATE ${STUB_SRC_DIR} ${STUB_SRC_DIR}/appkit)
target_link_libraries(stub-scriptingbridge PRIVATE stub-flags GNUStep sdl2)
