# Riot game-specific stubs — RPatch/mvg, League only
add_library(stub-game-riot OBJECT
    stub_riot.c
)
target_include_directories(stub-game-riot PRIVATE ${STUB_SRC_DIR})
target_link_libraries(stub-game-riot PRIVATE stub-flags GNUStep sdl2)
