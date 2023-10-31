#!/bin/bash
# This is similar to buildbot.sh but builds plugins for the game instead
# Some variables must be set before invoking this script. See buildtestplugin.sh
cd $ROOT
echo $(pwd)
FILES=$(find . -type f -name "*.c" | tr '\n' ' ')
FLAGS="-shared -fPIC -O1 -s -std=c99"
echo $FILES

# ----------------------------- compile linux
NIX32_CC="gcc -m32"
NIX64_CC="gcc -m64"
NIX_FLAGS="-nostartfiles -Wl,--entry=0"

echo "Compiling linux32.."
$NIX32_CC $FILES -I ~/client/src/ -I ./src/ -o ${PLUGIN}_nix32.so $FLAGS $NIX_FLAGS

echo "Compiling linux64.."
$NIX64_CC $FILES -I ~/client/src/ -I ./src/ -o ${PLUGIN}_nix64.so $FLAGS $NIX_FLAGS

# ----------------------------- compile macOS
MAC32_CC=~/osx/target/bin/o32-clang
MAC64_CC=~/osx/target/bin/o64-clang
MAC_FLAGS="-undefined dynamic_lookup"

echo "Compiling mac32.."
$MAC32_CC $FILES -I ~/client/src/ -I ./src/ -o ${PLUGIN}_mac32.dylib $FLAGS $MAC_FLAGS

echo "Compiling mac64.."
$MAC64_CC $FILES -I ~/client/src/ -I ./src/ -o ${PLUGIN}_mac64.dylib $FLAGS $MAC_FLAGS

# ----------------------------- compile Windows
WIN32_CC=i686-w64-mingw32-gcc
WIN64_CC=x86_64-w64-mingw32-gcc
# TODO: Figure out why we sometimes need this with mingw
# If we don't include nostart files for some plugins, get a
#   ertr000001.o:(.rdata+0x0): undefined reference to `_pei386_runtime_relocator'
# Seems to happen if you use exported variables from the game
if [ -z "$LITE_MODE" ]; then
        WIN_FLAGS=""
else
        WIN_FLAGS="-nostartfiles -Wl,--entry=0"
fi
# NOTE: You also need to install 'mingw-w64-tools' package for gendef

echo "Compiling win32"
rm ClassiCube.exe ClassiCube.def
cp ~/client/src/cc-w32-d3d.exe ClassiCube.exe
gendef ClassiCube.exe
i686-w64-mingw32-dlltool -d ClassiCube.def -l libClassiCube.a -D ClassiCube.exe
$WIN32_CC $FILES -I ~/client/src/ -I ./src/ -o ${PLUGIN}_win32.dll $FLAGS $WIN_FLAGS -L . -lClassiCube

echo "Compiling win64"
rm ClassiCube.exe ClassiCube.def
cp ~/client/src/cc-w64-d3d.exe ClassiCube.exe
gendef ClassiCube.exe
x86_64-w64-mingw32-dlltool -d ClassiCube.def -l libClassiCube.a -D ClassiCube.exe
$WIN64_CC $FILES -I ~/client/src/ -I ./src/ -o ${PLUGIN}_win64.dll $FLAGS $WIN_FLAGS -L . -lClassiCube

# ----------------------------- compile raspberry pi
RPI_CC=~/rpi/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin/arm-linux-gnueabihf-gcc-4.8.3
RPI_FLAGS="-nostartfiles -Wl,--entry=0"

echo "Compiling rpi"
$RPI_CC $FILES -I ~/client/src/ -I ./src/ -o ${PLUGIN}_rpi.so $FLAGS $RPI_FLAGS