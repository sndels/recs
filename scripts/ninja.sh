#!/usr/bin/env sh

CC=clang-18 CXX=clang++-18 cmake \
    -DCMAKE_BUILD_TYPE=$1\
    -DCMAKE_CXX_FLAGS=-fdiagnostics-color=always\
    -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=mold" \
    -G Ninja \
    -B build
err=$?

cd build

if [ "$err" -eq "0" ]; then
ninja test
err=$?
fi

if [ "$err" -eq "0" ]; then
./test
err=$?
fi

cd ..
