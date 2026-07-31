# Security — SecureTransport/trust (fail-clean)
add_library(stub-security OBJECT
    stub_securetransport.c
)
target_include_directories(stub-security PRIVATE ${STUB_SRC_DIR})
target_link_libraries(stub-security PRIVATE stub-flags GNUStep sdl2)
