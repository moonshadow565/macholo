project(LANGUAGES C ASM OBJC)

add_subdirectory(gnustep)

find_package(SDL2 REQUIRED)
add_library(sdl2 INTERFACE)
target_include_directories(sdl2 INTERFACE ${SDL2_INCLUDE_DIRS})
target_link_libraries(sdl2 INTERFACE ${SDL2_LIBRARIES})

# Shared by every stub module OBJECT lib. PIC is not implied for OBJECT libs the way
# it was when these sources lived directly in the SHARED target, so ask for it here.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(STUB_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_library(stub-flags INTERFACE)
target_compile_options(stub-flags INTERFACE -Wno-nullability-completeness -Wno-c23-extensions)

# Game-specific modules (game-riot; game-blizzard once it exists) are opt-in: default
# builds include them (harmless, identical exports), but a per-game/CI build can drop
# them. See docs/REFACTOR.md "Game isolation".
option(MACHOLO_GAME_STUBS "Build game-specific stub modules (game-riot)" ON)

# One OBJECT lib per framework/module; see docs/REFACTOR.md track 1.
add_subdirectory(src/appkit)
add_subdirectory(src/arch-x86_64)
add_subdirectory(src/audiotoolbox)
add_subdirectory(src/carbon)
add_subdirectory(src/cg)
add_subdirectory(src/corefoundation)
add_subdirectory(src/coregraphics)
add_subdirectory(src/foundation)
if(MACHOLO_GAME_STUBS)
    add_subdirectory(src/game-riot)
endif()
add_subdirectory(src/iokit)
add_subdirectory(src/libsystem)
add_subdirectory(src/objc-rt)
add_subdirectory(src/scriptingbridge)
add_subdirectory(src/security)

set(STUB_GAME_OBJECTS "")
if(MACHOLO_GAME_STUBS)
    list(APPEND STUB_GAME_OBJECTS $<TARGET_OBJECTS:stub-game-riot>)
endif()

# The modules are aggregated into one SHARED lib so the loader's flat
# dlsym(nullptr, ...) namespace and the exported surface stay unchanged.
add_library(stub SHARED
    src/common.h

    $<TARGET_OBJECTS:stub-appkit>
    $<TARGET_OBJECTS:stub-arch-x86_64>
    $<TARGET_OBJECTS:stub-audiotoolbox>
    $<TARGET_OBJECTS:stub-carbon>
    $<TARGET_OBJECTS:stub-cg>
    $<TARGET_OBJECTS:stub-corefoundation>
    $<TARGET_OBJECTS:stub-coregraphics>
    $<TARGET_OBJECTS:stub-foundation>
    ${STUB_GAME_OBJECTS}
    $<TARGET_OBJECTS:stub-iokit>
    $<TARGET_OBJECTS:stub-libsystem>
    $<TARGET_OBJECTS:stub-objc-rt>
    $<TARGET_OBJECTS:stub-scriptingbridge>
    $<TARGET_OBJECTS:stub-security>
)
set_target_properties(stub PROPERTIES LINKER_LANGUAGE OBJC)
target_include_directories(stub PRIVATE src/)
target_link_libraries(stub PUBLIC GNUStep sdl2 GL)
# See link-hidden-syms.map: keeps libstub.so's exported surface stable across the module
# split by hiding GNUstep ObjC-runtime section-boundary boilerplate that would otherwise
# leak into the dynamic symbol table once Misc.m's ObjC content moved into more than one
# translation unit (docs/REFACTOR.md Stage 2b report).
target_link_options(stub PRIVATE "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/link-hidden-syms.map")
set_target_properties(stub PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/link-hidden-syms.map)
