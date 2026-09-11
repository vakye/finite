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

SourceFile="code/main.c"
OutputFile="build/finite"

CompileFlags=" \
    -g \
    -O0 \
    -std=gnu11 \
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
    -Wl,-lwayland-client \
    -Wl,-lxkbcommon"

clang $CompileFlags $SourceFile $LinkFlags

if [ $? -eq 0 ]; then
    echo "$(basename $SourceFile)";
fi

