# This is similar to buildbot.sh but builds plugins for the game instead
# Some variables must be set before invoking this script. See buildtestplugin.sh
cd $ROOT
echo $(pwd)
FILES=$(find . -type f -name "*.c" | tr '\n' ' ')
FLAGS="-shared -fPIC -O1 -s -std=c99"

NIX32_PATH="gcc -m32"
NIX64_PATH="gcc -m64"
MAC32_PATH="/home/buildbot/osx/target/bin/o32-clang"
MAC64_PATH="/home/buildbot/osx/target/bin/o64-clang"
WIN32_PATH="i686-w64-mingw32-gcc"
WIN64_PATH="x86_64-w64-mingw32-gcc"

echo $FILES
echo $INCLUDE_DIRS

echo "Compiling nix64"
$NIX64_PATH $FILES -I ~/client/src/ -I ./src/ -o ${PLUGIN}_nix64.so $FLAGS -nostartfiles -Wl,--entry=0

echo "Compiling nix32"
$NIX32_PATH $FILES -I ~/client/src/ -I ./src/ -o ${PLUGIN}_nix32.so $FLAGS -nostartfiles -Wl,--entry=0

echo "Compiling mac64"
$MAC64_PATH $FILES -I ~/client/src/ -I ./src/ -o ${PLUGIN}_osx64.so $FLAGS -undefined dynamic_lookup

echo "Compiling mac32"
$MAC32_PATH $FILES -I ~/client/src/ -I ./src/ -o ${PLUGIN}_osx32.so $FLAGS -undefined dynamic_lookup

echo "Compiling win32"
rm ClassiCube.exe ClassiCube.def
cp ~/client/src/cc-w32-d3d.exe ClassiCube.exe
gendef ClassiCube.exe
i686-w64-mingw32-dlltool -d ClassiCube.def -l libClassiCube.a -D ClassiCube.exe
$WIN32_PATH $FILES -I ~/client/src/ -I ./src/ -o ${PLUGIN}_win32.dll -nostartfiles -Wl,--entry=0 $FLAGS -L . -lClassiCube

echo "Compiling win64"
rm ClassiCube.exe ClassiCube.def
cp ~/client/src/cc-w64-d3d.exe ClassiCube.exe
gendef ClassiCube.exe
x86_64-w64-mingw32-dlltool -d ClassiCube.def -l libClassiCube.a -D ClassiCube.exe
$WIN64_PATH $FILES -I ~/client/src/ -I ./src/ -o ${PLUGIN}_win64.dll -nostartfiles -Wl,--entry=0 $FLAGS -L . -lClassiCube
