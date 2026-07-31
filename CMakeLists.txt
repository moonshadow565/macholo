cmake_minimum_required(VERSION 3.20)

project(macholo LANGUAGES ASM OBJC C CXX)

set(CMAKE_CXX_STANDARD_VERSION 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(LIBCXX_CXX_ABI libcxxabi)
set(CXX_FLAGS "-stdlib=libc++ -std=c++20")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -fuse-ld=lld -Wl,--allow-shlib-undefined -lc++abi")

# GNUstep dependencies. Default (empty GNUSTEP_DIR) builds the vendored libobjc2 +
# gnustep-base under vendor/; see docs/GNUSTEP-BUILD.md. Setting -DGNUSTEP_DIR=<dir>
# is the escape hatch back to a prebuilt sysroot (deps/build-sysroot.sh).
set(GNUSTEP_DIR "" CACHE STRING "Prebuilt GNUstep sysroot; empty = build vendor/ from source")
if(NOT GNUSTEP_DIR)
    add_subdirectory(vendor)
endif()

add_subdirectory(stub)

add_executable(macholo
    src/common.hpp
    src/common.cpp
    src/ak.cpp
    src/ak.hpp
    src/lib.cpp
    src/lib.hpp
    src/loader.hpp
    src/loader.cpp
    src/macho.hpp
    src/macho.cpp
    src/main.cpp
    src/new.hpp
    src/new.cpp
    src/objc.cpp
    src/objc.hpp
    src/tlv.cpp
    src/tlv.hpp
)
target_link_libraries(macholo PUBLIC stub asound)

# Parses like the loader (FAT slice + Object::read) but executes nothing; no stub/GNUstep deps.
add_executable(macholo-dump
    src/common.hpp
    src/common.cpp
    src/macho.hpp
    src/macho.cpp
    src/dump.cpp
)

