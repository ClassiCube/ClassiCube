FLAGS="-fPIC -shared -s -O1 -fvisibility=hidden -rdynamic"
LIBS="-lGLESv2 -lEGL -lOpenSLES -lm -landroid -llog"
NDK_ROOT="/home/buildbot/android/android-ndk-r22/toolchains/llvm/prebuilt/linux-x86_64/bin"
TOOLS_ROOT="/home/buildbot/android/sdk/build-tools/26.0.0"
SDK_ROOT="/home/buildbot/android/sdk/platforms/android-26"

cd /home/buildbot/client/src
$NDK_ROOT/armv7a-linux-androideabi16-clang *.c $FLAGS $LIBS -o cc-droid-arm_32
$NDK_ROOT/aarch64-linux-android21-clang *.c $FLAGS $LIBS -o cc-droid-arm_64
$NDK_ROOT/i686-linux-android16-clang *.c $FLAGS $LIBS -o cc-droid-x86_32
$NDK_ROOT/x86_64-linux-android21-clang *.c $FLAGS $LIBS -o cc-droid-x86_64


cd ../android/app/src/main
# remove old java temp files
rm -rf obj
mkdir obj
rm classes.dex

# copy required native libraries
rm -rf lib
mkdir lib
mkdir lib/armeabi-v7a
mkdir lib/arm64-v8a
mkdir lib/x86
mkdir lib/x86_64
cp ~/client/src/cc-droid-arm_32 lib/armeabi-v7a/libclassicube.so
cp ~/client/src/cc-droid-arm_64 lib/arm64-v8a/libclassicube.so
cp ~/client/src/cc-droid-x86_32 lib/x86/libclassicube.so
cp ~/client/src/cc-droid-x86_64 lib/x86_64/libclassicube.so

# The following commands are for manually building an .apk, see
# https://spin.atomicobject.com/2011/08/22/building-android-application-bundles-apks-by-hand/
# https://github.com/cnlohr/rawdrawandroid/blob/master/Makefile
# https://stackoverflow.com/questions/41132753/how-can-i-build-an-android-apk-without-gradle-on-the-command-line
# https://github.com/skanti/Android-Manual-Build-Command-Line/blob/master/hello-jni/Makefile
# https://github.com/skanti/Android-Manual-Build-Command-Line/blob/master/hello-jni/CMakeLists.txt

# compile interop java file into its multiple .class files
javac java/com/classicube/MainActivity.java -d ./obj -classpath $SDK_ROOT/android.jar
# compile the multiple .class files into one .dex file
$TOOLS_ROOT/dx --dex --output=obj/classes.dex ./obj
# create initial .apk with packaged version of resources
$TOOLS_ROOT/aapt package -f -M AndroidManifest.xml -S res -F obj/cc-unsigned.apk -I $SDK_ROOT/android.jar
# and add all the required files
cp obj/classes.dex classes.dex
$TOOLS_ROOT/aapt add -f obj/cc-unsigned.apk classes.dex lib/armeabi-v7a/libclassicube.so lib/arm64-v8a/libclassicube.so lib/x86/libclassicube.so lib/x86_64/libclassicube.so
# sign the apk with debug key (https://stackoverflow.com/questions/16711233/)
cp obj/cc-unsigned.apk obj/cc-signed.apk
jarsigner -verbose -sigalg SHA1withRSA -digestalg SHA1 -keystore debug.keystore -storepass android -keypass android obj/cc-signed.apk androiddebugkey
# create aligned .apk file
$TOOLS_ROOT/zipalign -f -v 4 obj/cc-signed.apk obj/cc-final.apk