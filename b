#!/usr/bin/env bash
set -e

cmake -S . -B build -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=toolchain/x86_64-elf.cmake

cmake --build build
./scripts/build_iso.sh
