#!/bin/sh
set -e
cd /sources
rm -rf mesa-25.1.8
tar xf mesa-25.1.8.tar.xz
cd mesa-25.1.8
mkdir build
cd build
meson setup .. --prefix=/usr --buildtype=release -D platforms=x11 -D gallium-drivers=iris,crocus,i915,r300,r600,radeonsi,nouveau,virgl,svga,llvmpipe,softpipe -D vulkan-drivers= -D valgrind=disabled -D libunwind=disabled -D glvnd=disabled -D gles1=enabled -D gles2=enabled -D llvm=enabled
ninja
ninja install
echo MESA-FINITO
