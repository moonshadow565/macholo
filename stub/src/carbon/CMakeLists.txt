# Carbon — TIS/UCKeyTranslate/FSRef
add_library(stub-carbon OBJECT
    stub_carbon.m
)
target_include_directories(stub-carbon PRIVATE ${STUB_SRC_DIR})
target_link_libraries(stub-carbon PRIVATE stub-flags GNUStep sdl2)
