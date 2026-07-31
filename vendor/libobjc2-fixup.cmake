# Injected into vendor/libobjc2/CMakeLists.txt immediately after its project()
# call, via CMAKE_PROJECT_libobjc_INCLUDE (see vendor/CMakeLists.txt).
#
# libobjc2 assumes it is the top-level project.  Its eh_trampoline.S rule -- the
# one that compiles eh_trampoline.cc and renames __gxx_personality_v0 to
# test_eh_personality, i.e. the ObjC<->C++ exception personality probe -- spells
# its input as ${CMAKE_SOURCE_DIR}/eh_trampoline.cc and redirects its output to
# ${CMAKE_BINARY_DIR}/eh_trampoline.S, while add_custom_command(OUTPUT ...)
# resolves eh_trampoline.S against CMAKE_CURRENT_BINARY_DIR.  Standalone all
# three agree; under add_subdirectory the first two resolve to macholo's root.
#
# Re-pointing the two variables here is preferred over editing the vendored
# CMakeLists, which must stay byte-identical to upstream v2.3.  project() resets
# them, which is why this cannot live in the parent scope.
set(CMAKE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")
