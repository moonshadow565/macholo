#!/bin/bash
set -x

# Based on: https://github.com/plaurent/gnustep-build/

export CC=clang-18
export CXX=clang++-18
export CXXFLAGS="-std=c++11 -stdlib=libc++"
export LDXXFLAGS="-stdlib=libc++ -lc++abi -lsupc++"
export RUNTIME_VERSION=gnustep-2.1
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
export LD=lld-18
export LDFLAGS="-fuse-ld=lld-18 -L/usr/local/lib"
WORKDIR="workdir"

function install_deps()
{
  sudo apt update
  sudo apt-get update
  sudo apt -y install \
    build-essential \
    wget \
    git \
    subversion \
    cmake \
    libffi-dev \
    libxml2-dev \
    libgnutls28-dev \
    libicu-dev \
    libblocksruntime-dev \
    libkqueue-dev \
    libpthread-workqueue-dev \
    autoconf \
    libtool \
    libjpeg-dev \
    libtiff-dev \
    libffi-dev \
    libcairo-dev \
    libx11-dev \
    libxt-dev \
    libxft-dev \
    libxrandr-dev
}

function fetch_sources()
{
  rm -rf $WORKDIR
  mkdir $WORKDIR
  pushd $WORKDIR
  git clone https://github.com/apple/swift-corelibs-libdispatch
  git clone --recursive https://github.com/gnustep/libobjc2.git
  git clone https://github.com/gnustep/tools-make.git
  git clone https://github.com/gnustep/libs-base.git
  git clone https://github.com/gnustep/libs-corebase.git
  git clone https://github.com/gnustep/libs-gui.git
  git clone https://github.com/gnustep/libs-back.git
  popd
}

function build_with_configure()
{
  rm -rf build
  git clone .git build
  cd build
  ./configure "$@"
  make -j8
  sudo -E make install
}

function build_with_cmake()
{
  rm -rf build
  mkdir build
  cd build
  cmake .. \
    -DCMAKE_C_COMPILER="$CC" \
    -DCMAKE_CXX_COMPILER="$CXX" \
    -DCMAKE_ASM_COMPILER="$CC" \
    -DCMAKE_CXX_FLAGS="$CXXFLAGS" \
    -DCMAKE_EXE_LINKER_FLAGS="$LDXXFLAGS" \
    -DCMAKE_SHARED_LINKER_FLAGS="$LDXXFLAGS" \
    -DCMAKE_BUILD_TYPE=Release \
    -DTESTS=OFF \
    "$@"
  make -j8
  sudo -E make install
}

function build_0_toolsmake_bootstrap()
{
  pushd $WORKDIR/tools-make
  build_with_configure \
    --with-layout=gnustep \
    --disable-importing-config-file \
    --enable-native-objc-exceptions \
    --enable-objc-arc \
    --enable-install-ld-so-conf \
    --with-library-combo=ng-gnu-gnu
  popd
}

function build_1_libdispatch()
{
  pushd $WORKDIR/swift-corelibs-libdispatch
  build_with_cmake
  sudo ldconfig
  popd
}

function build_2_libobjc2()
{
  pushd $WORKDIR/libobjc2
  build_with_cmake
  sudo ldconfig
  popd
}

function build_3_toolsmake()
{
  pushd $WORKDIR/tools-make
  build_with_configure \
    --with-layout=gnustep \
    --disable-importing-config-file \
    --enable-native-objc-exceptions \
    --enable-objc-arc \
    --enable-install-ld-so-conf \
    --with-library-combo=ng-gnu-gnu
  popd
}

function build_4_libsbase()
{
  pushd $WORKDIR/libs-base
  build_with_configure
  popd
}

function build_5_libscorebase()
{
  pushd $WORKDIR/libs-corebase
  build_with_configure
  sudo ldconfig
  popd
}

function build_6_libsgui()
{
  pushd $WORKDIR/libs-gui
  build_with_configure
  popd
}

function build_7_libsback()
{
  pushd $WORKDIR/libs-back
  build_with_configure
  popd
}

# Can comment this out if already installed:
install_deps
# Can comment this out if already fetched:
fetch_sources

build_0_toolsmake_bootstrap
build_1_libdispatch
build_2_libobjc2
build_3_toolsmake
 . /usr/GNUstep/System/Library/Makefiles/GNUstep.sh
build_4_libsbase
CFLAGS=`gnustep-config --objc-flags` build_5_libscorebase
build_6_libsgui
build_7_libsback
