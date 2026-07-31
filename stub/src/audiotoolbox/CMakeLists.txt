# AudioToolbox — AUGraph & co
add_library(stub-audiotoolbox OBJECT
    stub_audio.c
)
target_include_directories(stub-audiotoolbox PRIVATE ${STUB_SRC_DIR})
target_link_libraries(stub-audiotoolbox PRIVATE stub-flags GNUStep sdl2)
