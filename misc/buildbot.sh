#!/bin/bash

# These build instructions are for ubuntu. Other linux distributions may have saner behaviour.
# The build script relies on the following assumptions:
# - You have already cloned ClassiCube from github
# - The root folder is ~/client/ (i.e. folder layout is ~/client/.git/, ~/client/src/, etc)
# First you need to install these packages: gcc, i686-w64-mingw32-gcc and x86_64-w64-mingw32-gcc
# 
# You then need to install these packages: libx11-dev and libgl1-mesa-dev
# - if 32 bit, also install the 64 bit variants of all these packages (e.g. libx11-dev:amd64)
# - if 64 bit, also install the 32 bit variants of all these packages (e.g. libx11-dev:i386)
#
# However! You may find that with some ubuntu versions, installing the alternate
#  bit variant of libgl1-mesa-dev uninstalls your current package. 
# To fix this, first reinstall the normal libgl1-mesa-dev package
# The alternate bit .so files should have been left behind in the mesa folder, so adding a symlink should make it compile again
# - for 32 bit: ln -sf /usr/lib/x86_64-linux-gnu/mesa/libGL.so.1 /usr/lib/x86_64-linux-gnu/libGL.so
# - for 64 bit: ln -sf /usr/lib/i386-linux-gnu/mesa/libGL.so.1 /usr/lib/i386-linux-gnu/libGL.so

ROOT_DIR=~/client # can be changed
ALL_FLAGS="-O1 -s -fno-stack-protector -fno-math-errno -Qn -w"

# ----------------------------- compile windows
#   I installed gcc-mingw-w64-i686 and gcc-mingw-w64-x86-64 packages
WIN32_CC="i686-w64-mingw32-gcc"
WIN64_CC="x86_64-w64-mingw32-gcc"
WIN32_FLAGS="-mwindows -nostartfiles -Wl,-e_main_real -DCC_NOMAIN"
WIN64_FLAGS="-mwindows -nostartfiles -Wl,-emain_real -DCC_NOMAIN"

build_win32() {
  echo "Building win32.."
  cp $ROOT_DIR/misc/CCicon_32.res $ROOT_DIR/src/CCicon_32.res
  rm cc-w32-d3d.exe cc-w32-ogl.exe

  $WIN32_CC *.c $ALL_FLAGS $WIN32_FLAGS -o cc-w32-d3d.exe CCicon_32.res -DCC_COMMIT_SHA=\"$LATEST\" -lws2_32 -lwininet -lwinmm -limagehlp -lcrypt32
  $WIN32_CC *.c $ALL_FLAGS $WIN32_FLAGS -o cc-w32-ogl.exe CCicon_32.res -DCC_COMMIT_SHA=\"$LATEST\" -DCC_BUILD_MANUAL -DCC_BUILD_WIN -DCC_BUILD_GL -DCC_BUILD_WINGUI -DCC_BUILD_WGL -DCC_BUILD_WINMM -DCC_BUILD_WININET -lws2_32 -lwininet -lwinmm -limagehlp -lcrypt32 -lopengl32
}

build_win64() {
  echo "Building win64.."
  cp $ROOT_DIR/misc/CCicon_64.res $ROOT_DIR/src/CCicon_64.res
  rm cc-w64-d3d.exe cc-w64-ogl.exe
  
  $WIN64_CC *.c $ALL_FLAGS $WIN64_FLAGS -o cc-w64-d3d.exe CCicon_64.res -DCC_COMMIT_SHA=\"$LATEST\" -lws2_32 -lwininet -lwinmm -limagehlp -lcrypt32
  $WIN64_CC *.c $ALL_FLAGS $WIN64_FLAGS -o cc-w64-ogl.exe CCicon_64.res -DCC_COMMIT_SHA=\"$LATEST\" -DCC_BUILD_MANUAL -DCC_BUILD_WIN -DCC_BUILD_GL -DCC_BUILD_WINGUI -DCC_BUILD_WGL -DCC_BUILD_WINMM -DCC_BUILD_WININET -lws2_32 -lwininet -lwinmm -limagehlp -lcrypt32 -lopengl32
}

# ----------------------------- compile linux
#   I installed gcc and gcc-multilib packages
NIX32_FLAGS="-no-pie -fno-pie -m32 -fvisibility=hidden -rdynamic -DCC_BUILD_ICON"
NIX64_FLAGS="-no-pie -fno-pie -m64 -fvisibility=hidden -rdynamic -DCC_BUILD_ICON"

build_nix32() {
  echo "Building linux32.."
  cp $ROOT_DIR/misc/CCicon_nix32 $ROOT_DIR/src/CCicon_nix32.o
  rm cc-nix32
  gcc *.c $ALL_FLAGS $NIX32_FLAGS CCicon_nix32.o -DCC_COMMIT_SHA=\"$LATEST\" -o cc-nix32 -lX11 -lXi -lpthread -lGL -lm -ldl
}

build_nix64() {
  echo "Building linux64.."
  cp $ROOT_DIR/misc/CCicon_nix64 $ROOT_DIR/src/CCicon_nix64.o
  rm cc-nix64
  gcc *.c $ALL_FLAGS $NIX64_FLAGS CCicon_nix64.o -DCC_COMMIT_SHA=\"$LATEST\" -o cc-nix64 -lX11 -lXi -lpthread -lGL -lm -ldl
}

# ----------------------------- compile macOS
MAC32_CC="/home/buildbot/osx/target/bin/o32-clang"
MAC64_CC="/home/buildbot/osx/target/bin/o64-clang"
MACOS_FLAGS="-fvisibility=hidden -rdynamic -DCC_BUILD_ICON"

build_mac32() {
  echo "Building mac32.."
  cp $ROOT_DIR/misc/CCicon_mac32 $ROOT_DIR/src/CCicon_mac32.o
  rm cc-osx32
  $MAC32_CC *.c $ALL_FLAGS $MACOS_FLAGS CCicon_mac32.o -DCC_COMMIT_SHA=\"$LATEST\" -o cc-osx32 -framework Carbon -framework AGL -framework OpenGL -framework IOKit -lgcc_s.1
}

build_mac64() {
  echo "Building mac64.."
  cp $ROOT_DIR/misc/CCicon_mac64 $ROOT_DIR/src/CCicon_mac64.o
  rm cc-osx64
  $MAC64_CC *.c interop_cocoa.m $ALL_FLAGS $MACOS_FLAGS CCicon_mac64.o -DCC_COMMIT_SHA=\"$LATEST\" -o cc-osx64 -framework Cocoa -framework OpenGL -framework IOKit -lobjc
}

# ----------------------------- compile web
#   I installed emscripten per https://emscripten.org/docs/getting_started/downloads.html
WEB_CC="/home/buildbot/emsdk/emscripten/1.38.31/emcc"

build_web() {
  echo "Building web.."
  rm cc.js
  $WEB_CC *.c -O1 -o cc.js --js-library interop_web.js -s WASM=0 -s LEGACY_VM_SUPPORT=1 -s ALLOW_MEMORY_GROWTH=1 -s ABORTING_MALLOC=0 --preload-file texpacks/default.zip -w
  # so game loads textures from classicube.net/static/default.zip
  sed -i 's#cc.data#/static/default.zip#g' cc.js
  # fix mouse wheel scrolling page not being properly prevented
  # "[Intervention] Unable to preventDefault inside passive event listener due to target being treated as passive."
  sed -i 's#eventHandler.useCapture);#{ useCapture: eventHandler.useCapture, passive: false });#g' cc.js
}

# ----------------------------- compile raspberry pi
#   I cloned https://github.com/raspberrypi/tools to get prebuilt cross compilers
#   Then I copied across various files/folders from /usr/include and /usr/lib from a real Raspberry pi as needed
RPI_CC=~/rpi/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin/arm-linux-gnueabihf-gcc-4.8.3
RPI_FLAGS="-fvisibility=hidden -rdynamic -DCC_BUILD_ICON -DCC_BUILD_RPI"

build_rpi() {
  echo "Building rpi.."
  cp $ROOT_DIR/misc/CCicon_rpi $ROOT_DIR/src/CCicon_rpi.o
  rm cc-rpi
  $RPI_CC *.c $ALL_FLAGS $RPI_FLAGS -I ~/rpi/include -L ~/rpi/lib CCicon_rpi.o -DCC_COMMIT_SHA=\"$LATEST\" -o cc-rpi -lGLESv2 -lEGL -lX11 -lXi -lm -lpthread -ldl -lrt -Wl,-rpath-link ~/rpi/lib
}

# ----------------------------- compile android
DROID_FLAGS="-fPIC -shared -s -O1 -fvisibility=hidden -rdynamic -funwind-tables"
DROID_LIBS="-lGLESv2 -lEGL -lm -landroid -llog"
NDK_ROOT="/home/buildbot/android/android-ndk-r22/toolchains/llvm/prebuilt/linux-x86_64/bin"
TOOLS_ROOT="/home/buildbot/android/sdk/build-tools/26.0.0"
SDK_ROOT="/home/buildbot/android/sdk/platforms/android-26"
  
build_android() {
  echo "Building android.."
  $NDK_ROOT/armv7a-linux-androideabi16-clang *.c $DROID_FLAGS -march=armv5 $DROID_LIBS -DCC_COMMIT_SHA=\"$LATEST\" -o cc-droid-arm_16
  $NDK_ROOT/armv7a-linux-androideabi16-clang *.c $DROID_FLAGS $DROID_LIBS -DCC_COMMIT_SHA=\"$LATEST\" -o cc-droid-arm_32
  $NDK_ROOT/aarch64-linux-android21-clang *.c $DROID_FLAGS $DROID_LIBS -DCC_COMMIT_SHA=\"$LATEST\" -o cc-droid-arm_64
  $NDK_ROOT/i686-linux-android16-clang *.c $DROID_FLAGS $DROID_LIBS -DCC_COMMIT_SHA=\"$LATEST\" -o cc-droid-x86_32
  $NDK_ROOT/x86_64-linux-android21-clang *.c $DROID_FLAGS $DROID_LIBS -DCC_COMMIT_SHA=\"$LATEST\" -o cc-droid-x86_64
  
  cd $ROOT_DIR/android/app/src/main
  # remove old java temp files
  rm -rf obj
  mkdir obj
  rm classes.dex
  # copy required native libraries
  rm -rf lib
  mkdir lib lib/armeabi lib/armeabi-v7a lib/arm64-v8a lib/x86 lib/x86_64
  cp $ROOT_DIR/src/cc-droid-arm_16 lib/armeabi/libclassicube.so
  cp $ROOT_DIR/src/cc-droid-arm_32 lib/armeabi-v7a/libclassicube.so
  cp $ROOT_DIR/src/cc-droid-arm_64 lib/arm64-v8a/libclassicube.so
  cp $ROOT_DIR/src/cc-droid-x86_32 lib/x86/libclassicube.so
  cp $ROOT_DIR/src/cc-droid-x86_64 lib/x86_64/libclassicube.so
  
  # The following commands are for manually building an .apk, see
  #  https://spin.atomicobject.com/2011/08/22/building-android-application-bundles-apks-by-hand/
  #  https://github.com/cnlohr/rawdrawandroid/blob/master/Makefile
  #  https://stackoverflow.com/questions/41132753/how-can-i-build-an-android-apk-without-gradle-on-the-command-line
  #  https://github.com/skanti/Android-Manual-Build-Command-Line/blob/master/hello-jni/Makefile
  #  https://github.com/skanti/Android-Manual-Build-Command-Line/blob/master/hello-jni/CMakeLists.txt
  
  # compile interop java file into its multiple .class files
  javac java/com/classicube/MainActivity.java -d ./obj -classpath $SDK_ROOT/android.jar
  # compile the multiple .class files into one .dex file
  $TOOLS_ROOT/dx --dex --output=obj/classes.dex ./obj
  # create initial .apk with packaged version of resources
  $TOOLS_ROOT/aapt package -f -M AndroidManifest.xml -S res -F obj/cc-unsigned.apk -I $SDK_ROOT/android.jar
  # and add all the required files
  cp obj/classes.dex classes.dex
  $TOOLS_ROOT/aapt add -f obj/cc-unsigned.apk classes.dex lib/armeabi/libclassicube.so lib/armeabi-v7a/libclassicube.so lib/arm64-v8a/libclassicube.so lib/x86/libclassicube.so lib/x86_64/libclassicube.so
  # sign the apk with debug key (https://stackoverflow.com/questions/16711233/)
  cp obj/cc-unsigned.apk obj/cc-signed.apk
  jarsigner -verbose -sigalg SHA1withRSA -digestalg SHA1 -keystore debug.keystore -storepass android -keypass android obj/cc-signed.apk androiddebugkey
  # create aligned .apk file
  $TOOLS_ROOT/zipalign -f 4 obj/cc-signed.apk $ROOT_DIR/src/cc.apk
}

# ----------------------------- driver
cd $ROOT_DIR/src/
echo $PWD
git pull https://github.com/UnknownShadow200/ClassiCube.git
git fetch --all
git reset --hard origin/master
LATEST=$(git rev-parse --short HEAD)

run_timed() {
  beg=`date +%s`
  $1
  end=`date +%s`

  runtime=$((end-beg))
  echo "Compiling took $runtime"
}

run_timed build_win32
run_timed build_win64
run_timed build_nix32
run_timed build_nix64
run_timed build_mac32
run_timed build_mac64
run_timed build_web
run_timed build_rpi
run_timed build_android

cd ~
python3 notify.py