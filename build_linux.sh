#!/bin/bash

if [ ! -d build ]; then
    mkdir -p build;
fi

shopt -s nullglob
cd ./code/shaders
for ShaderFile in *.{vert,frag,comp,mesh,task}; do
    glslangValidator --target-env vulkan1.3 -x $ShaderFile -o "${ShaderFile}.h"
done
cd ../..

SourceFile="code/linux_main.c"
ObjectFile="build/linux_main.o"
OutputFile="build/finite"

CompileFlags=" \
    -c \
    -O2 \
    -std=gnu11 \
    -ffreestanding \
    -fpie \
    -fno-stack-protector \
    -fno-strict-aliasing \
    -nostdlib \
    -Wall -Wextra -Wpedantic -Werror \
    -Wno-unused-parameter \
    -Wno-unused-variable \
    -Wno-unused-function \
    -Wno-unused-but-set-variable \
    -Wno-switch \
    -o $ObjectFile"

LinkFlags=" \
    -o $OutputFile \
    -L /usr/lib"

LinkLibraries=" \
    -lwayland-client \
    -lxkbcommon \
    -lasound"

clang $CompileFlags $SourceFile
clang $LinkFlags $ObjectFile $LinkLibraries

if [ $? -eq 0 ]; then
    echo "$(basename $SourceFile)";
fi

