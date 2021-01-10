#!/bin/bash

# These build instructions are for ubuntu. Other linux distributions may have saner behaviour.
# The build script relies on the following assumptions:
# - You have already cloned ClassiCube from github
# - The root folder is ~/client/ (i.e. folder layout is ~/client/.git/, ~/client/src/, etc)
# First you need to install these packages: gcc, i686-w64-mingw32-gcc and x86_64-w64-mingw32-gcc
# 
# You then need to install these packages: libx11-dev and libgl1-mesa-dev
# - if 32 bit, then install the 64 bit variants of all these packages (e.g. libx11-dev:amd64)
# - if 64 bit, then install the 32 bit variants of all these packages (e.g. libx11-dev:i386)
#
# However! You may find that installing the alternate bit variant of libgl1-mesa-dev uninstalls your current package
# To fix this, first reinstall the normal libgl1-mesa-dev package
# The alternate bit .so files should have been left behind in the mesa folder, so adding a symlink should make it compile again
# - for 32 bit: ln -sf /usr/lib/x86_64-linux-gnu/mesa/libGL.so.1 /usr/lib/x86_64-linux-gnu/libGL.so
# - for 64 bit: ln -sf /usr/lib/i386-linux-gnu/mesa/libGL.so.1 /usr/lib/i386-linux-gnu/libGL.so

# paths, change these as needed
SOURCE_DIR=~/client
WEB_CC="~/emsdk/emscripten/1.38.31/emcc"
MAC32_CC="~/osx/target/bin/o32-clang"
MAC64_CC="~/osx/target/bin/o64-clang"
WIN32_CC="i686-w64-mingw32-gcc"
WIN64_CC="x86_64-w64-mingw32-gcc"

# to simplify stuff
ALL_FLAGS="-O1 -s -fno-stack-protector -fno-math-errno -Qn -w"
WIN32_FLAGS="-mwindows -nostartfiles -Wl,-e_main_real -DCC_NOMAIN"
WIN64_FLAGS="-mwindows -nostartfiles -Wl,-emain_real -DCC_NOMAIN"
NIX32_FLAGS="-no-pie -fno-pie -m32 -fvisibility=hidden -rdynamic -DCC_BUILD_ICON"
NIX64_FLAGS="-no-pie -fno-pie -m64 -fvisibility=hidden -rdynamic -DCC_BUILD_ICON"
RPI32_FLAGS="-I ~/rpi/include -L ~/rpi/lib -fvisibility=hidden -rdynamic -DCC_BUILD_ICON"
MACOS_FLAGS="-fvisibility=hidden -rdynamic -DCC_BUILD_ICON"

# I cloned https://github.com/raspberrypi/tools to get prebuilt cross compilers
# Then I copied across various files/folders from /usr/include and /usr/lib from a real Raspberry pi as needed
RPI_CC=~/rpi/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin/arm-linux-gnueabihf-gcc-4.8.3

# -----------------------------
build_win32() {
  echo "Building win32.."
  cp $SOURCE_DIR/misc/CCicon_32.res $SOURCE_DIR/src/CCicon_32.res
  rm cc-w32-d3d.exe cc-w32-ogl.exe

  $WIN32_CC *.c $ALL_FLAGS $WIN32_FLAGS -o cc-w32-d3d.exe CCicon_32.res -DCC_COMMIT_SHA=\"$LATEST\" -lws2_32 -lwininet -lwinmm -limagehlp -lcrypt32 -ld3d9
  $WIN32_CC *.c $ALL_FLAGS $WIN32_FLAGS -o cc-w32-ogl.exe CCicon_32.res -DCC_COMMIT_SHA=\"$LATEST\" -DCC_BUILD_MANUAL -DCC_BUILD_WIN -DCC_BUILD_GL -DCC_BUILD_WINGUI -DCC_BUILD_WGL -DCC_BUILD_WINMM -DCC_BUILD_WININET -lws2_32 -lwininet -lwinmm -limagehlp -lcrypt32 -lopengl32
}

build_win64() {
  echo "Building win64.."
  cp $SOURCE_DIR/misc/CCicon_64.res $SOURCE_DIR/src/CCicon_64.res
  rm cc-w64-d3d.exe cc-w64-ogl.exe
  
  $WIN64_CC *.c $ALL_FLAGS $WIN64_FLAGS -o cc-w64-d3d.exe CCicon_64.res -DCC_COMMIT_SHA=\"$LATEST\" -lws2_32 -lwininet -lwinmm -limagehlp -lcrypt32 -ld3d9
  $WIN64_CC *.c $ALL_FLAGS $WIN64_FLAGS -o cc-w64-ogl.exe CCicon_64.res -DCC_COMMIT_SHA=\"$LATEST\" -DCC_BUILD_MANUAL -DCC_BUILD_WIN -DCC_BUILD_GL -DCC_BUILD_WINGUI -DCC_BUILD_WGL -DCC_BUILD_WINMM -DCC_BUILD_WININET -lws2_32 -lwininet -lwinmm -limagehlp -lcrypt32 -lopengl32
}

build_nix32() {
  echo "Building linux32.."
  cp $SOURCE_DIR/misc/CCicon_nix32 $SOURCE_DIR/src/CCicon_nix32.o
  rm cc-nix32
  gcc *.c $ALL_FLAGS $NIX32_FLAGS CCicon_nix32.o -DCC_COMMIT_SHA=\"$LATEST\" -o cc-nix32 -lX11 -lXi -lpthread -lGL -lm -ldl
}

build_nix64() {
  echo "Building linux64.."
  cp $SOURCE_DIR/misc/CCicon_nix64 $SOURCE_DIR/src/CCicon_nix64.o
  rm cc-nix64
  gcc *.c $ALL_FLAGS $NIX64_FLAGS CCicon_nix64.o -DCC_COMMIT_SHA=\"$LATEST\" -o cc-nix64 -lX11 -lXi -lpthread -lGL -lm -ldl
}

build_mac32() {
  echo "Building mac32.."
  cp $SOURCE_DIR/misc/CCicon_mac32 $SOURCE_DIR/src/CCicon_mac32.o
  rm cc-osx32
  $MAC32_CC *.c $ALL_FLAGS $MACOS_FLAGS CCicon_mac32.o -DCC_COMMIT_SHA=\"$LATEST\" -o cc-osx32 -framework Carbon -framework AGL -framework OpenGL -framework IOKit -lgcc_s.1
}

build_mac64() {
  echo "Building mac64.."
  cp $SOURCE_DIR/misc/CCicon_mac64 $SOURCE_DIR/src/CCicon_mac64.o
  rm cc-osx64
  $MAC64_CC *.c $ALL_FLAGS $MACOS_FLAGS CCicon_mac64.o -DCC_COMMIT_SHA=\"$LATEST\" -o cc-osx64 -framework Cocoa -framework OpenGL -framework IOKit -lobjc
}

build_web() {
  echo "Building web.."
  rm cc.js
  $WEB_CC *.c -O1 -o cc.js -s WASM=0 -s LEGACY_VM_SUPPORT=1 -s ALLOW_MEMORY_GROWTH=1 -s ABORTING_MALLOC=0 --preload-file texpacks/default.zip -w
  # so game loads textures from classicube.net/static/default.zip
  sed -i 's#cc.data#/static/default.zip#g' cc.js
  # fix mouse wheel scrolling page not being properly prevented
  # "[Intervention] Unable to preventDefault inside passive event listener due to target being treated as passive."
  sed -i 's#eventHandler.useCapture);#{ useCapture: eventHandler.useCapture, passive: false });#g' cc.js
}

build_rpi() {
  echo "Building rpi.."
  cp $SOURCE_DIR/misc/CCicon_rpi $SOURCE_DIR/src/CCicon_rpi.o
  rm cc-rpi
  $RPI_CC *.c $ALL_FLAGS $RPI32_FLAGS CCicon_rpi.o -DCC_COMMIT_SHA=\"$LATEST\" -o cc-rpi -DCC_BUILD_RPI -lGLESv2 -lEGL -lX11 -lXi -lm -lpthread -ldl -lrt -Wl,-rpath-link ~/rpi/lib
}

# -----------------------------
cd $SOURCE_DIR/src/
echo $PWD
git pull https://github.com/UnknownShadow200/ClassiCube.git
git fetch --all
git reset --hard origin/master
LATEST=`git rev-parse --short HEAD`

build_win32
build_win64
build_nix32
build_nix64
build_mac32
build_mac64
build_web
build_rpi

cd ~
python3 notify.py