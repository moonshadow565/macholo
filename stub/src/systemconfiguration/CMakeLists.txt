# SystemConfiguration — SCDynamicStore surface
add_library(stub-systemconfiguration OBJECT
    SCDynamicStoreStubs.c
)
target_include_directories(stub-systemconfiguration PRIVATE ${STUB_SRC_DIR})
target_link_libraries(stub-systemconfiguration PRIVATE stub-flags GNUStep sdl2)
