# Package config shim for the vendored, header-only tsl::robin_map.
#
# libobjc2's CMakeLists does:
#
#     find_package(tsl-robin-map)
#     if (NOT tsl-robin-map_FOUND)
#         FetchContent_Declare(robinmap GIT_REPOSITORY https://github.com/... )
#         FetchContent_MakeAvailable(robinmap)
#     endif()
#
# i.e. a *configure-time git clone* whenever the host has no robin-map-devel.
# That is the only thing in the tree that would reach the network during
# `cmake -B build`, and the vendored file must stay byte-identical to v2.3, so it
# cannot be patched out.  Instead vendor/CMakeLists.txt points `tsl-robin-map_DIR`
# at this directory, the find_package succeeds in CONFIG mode, and the FetchContent
# branch is never taken.
#
# Deliberately NOT `add_subdirectory(robin-map)`: upstream's CMakeLists exists only
# to declare the same INTERFACE target plus a pile of install()/export() rules and a
# `tests` subdirectory, none of which this tree wants.  Six lines here are the whole
# of what libobjc2 consumes.

if(NOT TARGET tsl::robin_map)
    add_library(tsl::robin_map INTERFACE IMPORTED)
    set_target_properties(tsl::robin_map PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/../robin-map/include")
endif()

set(tsl-robin-map_FOUND TRUE)
