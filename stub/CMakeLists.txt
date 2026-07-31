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

# One OBJECT lib per framework/module; see docs/REFACTOR.md track 1.
add_subdirectory(src/appkit)
add_subdirectory(src/arch-x86_64)
add_subdirectory(src/audiotoolbox)
add_subdirectory(src/carbon)
add_subdirectory(src/cg)
add_subdirectory(src/corefoundation)
add_subdirectory(src/coregraphics)
add_subdirectory(src/foundation)
add_subdirectory(src/game-riot)
add_subdirectory(src/iokit)
add_subdirectory(src/libsystem)
add_subdirectory(src/objc-rt)
add_subdirectory(src/scriptingbridge)
add_subdirectory(src/security)

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
    $<TARGET_OBJECTS:stub-game-riot>
    $<TARGET_OBJECTS:stub-iokit>
    $<TARGET_OBJECTS:stub-libsystem>
    $<TARGET_OBJECTS:stub-objc-rt>
    $<TARGET_OBJECTS:stub-scriptingbridge>
    $<TARGET_OBJECTS:stub-security>
)
set_target_properties(stub PROPERTIES LINKER_LANGUAGE OBJC)
target_include_directories(stub PRIVATE src/)
target_link_libraries(stub PUBLIC GNUStep sdl2 GL)
