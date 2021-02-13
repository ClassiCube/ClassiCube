#!/bin/bash
# This is similar to buildbot.sh but builds plugins for the game instead
# Some variables must be set before invoking this script. See buildtestplugin.sh
cd $ROOT
echo $(pwd)
FILES=$(find . -type f -name "*.c" | tr '\n' ' ')
FLAGS="-shared -fPIC -O1 -s -std=c99"

NIX32_PATH="gcc -m32"
NIX64_PATH="gcc -m64"
MAC32_PATH=~/osx/target/bin/o32-clang
MAC64_PATH=~/osx/target/bin/o64-clang
WIN32_PATH=i686-w64-mingw32-gcc
WIN64_PATH=x86_64-w64-mingw32-gcc
RPI_PATH=~/rpi/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/bin/arm-linux-gnueabihf-gcc-4.8.3

echo $FILES
NIX_FLAGS="-nostartfiles -Wl,--entry=0"
MAC_FLAGS="-undefined dynamic_lookup"
# TODO: Figure out why we sometimes need this with mingw
# If we don't include nostart files for some plugins, get a
#   ertr000001.o:(.rdata+0x0): undefined reference to `_pei386_runtime_relocator'
# Seems to happen if you use exported variables from the game
if [ -z "$LITE_MODE" ]; then
        WIN_FLAGS=""
else
        WIN_FLAGS="-nostartfiles -Wl,--entry=0"
fi

echo "Compiling nix32"
$NIX32_PATH $FILES -I ~/client/src/ -I ./src/ -o ${PLUGIN}_nix32.so $FLAGS $NIX_FLAGS

echo "Compiling nix64"
$NIX64_PATH $FILES -I ~/client/src/ -I ./src/ -o ${PLUGIN}_nix64.so $FLAGS $NIX_FLAGS

echo "Compiling mac32"
$MAC32_PATH $FILES -I ~/client/src/ -I ./src/ -o ${PLUGIN}_mac32.dylib $FLAGS $MAC_FLAGS

echo "Compiling mac64"
$MAC64_PATH $FILES -I ~/client/src/ -I ./src/ -o ${PLUGIN}_mac64.dylib $FLAGS $MAC_FLAGS

echo "Compiling win32"
rm ClassiCube.exe ClassiCube.def
cp ~/client/src/cc-w32-d3d.exe ClassiCube.exe
gendef ClassiCube.exe
i686-w64-mingw32-dlltool -d ClassiCube.def -l libClassiCube.a -D ClassiCube.exe
$WIN32_PATH $FILES -I ~/client/src/ -I ./src/ -o ${PLUGIN}_win32.dll $FLAGS $WIN_FLAGS -L . -lClassiCube

echo "Compiling win64"
rm ClassiCube.exe ClassiCube.def
cp ~/client/src/cc-w64-d3d.exe ClassiCube.exe
gendef ClassiCube.exe
x86_64-w64-mingw32-dlltool -d ClassiCube.def -l libClassiCube.a -D ClassiCube.exe
$WIN64_PATH $FILES -I ~/client/src/ -I ./src/ -o ${PLUGIN}_win64.dll $FLAGS $WIN_FLAGS -L . -lClassiCube

echo "Compiling rpi"
$RPI_PATH $FILES -I ~/client/src/ -I ./src/ -o ${PLUGIN}_rpi.so $FLAGS -nostartfiles -Wl,--entry=0

