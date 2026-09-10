#!/bin/bash

if [ ! -d build ]; then
    mkdir -p build;
fi

SourceFile="code/main.c"
OutputFile="build/finite"

CompileFlags=" \
    -g \
    -O0 \
    -std=c11 \
    -ffreestanding \
    -fpie \
    -fno-stack-protector \
    -Wall -Wextra -Wpedantic -Werror \
    -Wno-unused-parameter \
    -Wno-unused-variable \
    -Wno-unused-function
    -o $OutputFile"

LinkFlags=" \
    -fuse-ld=lld \
    -Wl,-lwayland-client"

clang $CompileFlags $SourceFile $LinkFlags

