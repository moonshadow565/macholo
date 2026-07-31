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

# Second landmine: on the FIRST configure of a fresh build dir, .m files inside
# this directory classify as OBJC and libobjc2's
# $<COMPILE_LANGUAGE:OBJC>:-fobjc-runtime=gnustep-2.0 applies. On any RE-configure
# of an existing dir, project(libobjc C ASM CXX) enables C before OBJC exists in
# the cache-restored language set and the same .m files silently reclassify as C
# -- the runtime flag vanishes, associate.m/properties.m/fast_paths.m compile
# against the legacy GCC runtime, and every binary aborts at startup with
# "Version 2 Objective-C ABI may not be mixed with earlier versions."
# Pin the classification explicitly; properties set here (directory scope, before
# the targets are defined) stick to the vendored sources without editing them.
enable_language(OBJC)
file(GLOB _macholo_libobjc2_m "${CMAKE_CURRENT_SOURCE_DIR}/*.m")
set_source_files_properties(${_macholo_libobjc2_m} PROPERTIES LANGUAGE OBJC)
