#!/usr/bin/env sh

CC=clang-18 CXX=clang++-18 cmake \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -G Ninja \
    -S . \
    -B build-analysis
