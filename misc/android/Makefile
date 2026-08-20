TARGET_NAME	:= ClassiCube
SOURCE_DIRS	:= src/android
BUILD_DIR	:= build/android

SIGNED_APK	:= cc-signed.apk
UNSIGNED_APK:= cc-unsigned.apk

default: $(SIGNED_APK)


#---------------------------------------------------------------------------------
# Toolchain paths
#---------------------------------------------------------------------------------
NDK_ROOT	?= /opt/android-sdk-linux/ndk/25.2.9519653
TCHAIN_ROOT	:= $(NDK_ROOT)/toolchains/llvm/prebuilt/linux-x86_64/bin
TOOLS_ROOT	?= $(PWD)/build-tools
SDK_ROOT	?= /opt/android-sdk-linux/platforms/android-34


#---------------------------------------------------------------------------------
# Code generation
#--------------------------------------------------------------------------------
CFLAGS	:= -fPIC -fvisibility=hidden -funwind-tables \
			-O2 -fno-stack-protector -fno-math-errno -fno-ident -Werror
LDFLAGS	:= -fPIC -shared -rdynamic -s -Werror
LIBS	:= -lGLESv2 -lEGL -lm -llog
# TODO set no-stack-protector via extra cflags


#---------------------------------------------------------------------------------
# executable generation - ARMv5
#---------------------------------------------------------------------------------
ARM_v5_DIR	:= $(APK_DIR)/lib/armeabi
ARM_v5_LIB	:= $(ARM_v5_DIR)/libclassicube.so
$(ARM_v5_LIB): | $(ARM_v5_DIR)

BUILD_ARCH  := armv5
TARGET		:= $(ARM_v5_LIB)
CC			:= $(TCHAIN_ROOT)/armv7a-linux-androideabi19-clang -march=armv5
LINK		:= $(TCHAIN_ROOT)/armv7a-linux-androideabi19-clang -rtlib=libgcc -L $(TOOLS_ROOT)/runtime
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# executable generation - ARM32
#---------------------------------------------------------------------------------
ARM_32_DIR	:= $(APK_DIR)/lib/armeabi-v7a
ARM_32_LIB	:= $(ARM_32_DIR)/libclassicube.so
$(ARM_32_LIB): | $(ARM_32_DIR)

BUILD_ARCH  := arm32
TARGET		:= $(ARM_32_LIB)
CC			:= $(TCHAIN_ROOT)/armv7a-linux-androideabi19-clang -mfpu=vfp3-d16
LINK		:= $(TCHAIN_ROOT)/armv7a-linux-androideabi19-clang
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# executable generation - ARM64
#---------------------------------------------------------------------------------
ARM_64_DIR	:= $(APK_DIR)/lib/arm64-v8a
ARM_64_LIB	:= $(ARM_64_DIR)/libclassicube.so
$(ARM_64_LIB): | $(ARM_64_DIR)

BUILD_ARCH  := arm64
TARGET		:= $(ARM_64_LIB)
CC			:= $(TCHAIN_ROOT)/aarch64-linux-android21-clang
LINK		:= $(TCHAIN_ROOT)/aarch64-linux-android21-clang
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# executable generation - x86_32
#---------------------------------------------------------------------------------
X86_32_DIR	:= $(APK_DIR)/lib/x86
X86_32_LIB	:= $(X86_32_DIR)/libclassicube.so
$(X86_32_LIB): | $(X86_32_DIR)

BUILD_ARCH  := x86_32
TARGET		:= $(X86_32_LIB)
CC			:= $(TCHAIN_ROOT)/i686-linux-android21-clang
LINK		:= $(TCHAIN_ROOT)/i686-linux-android21-clang
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# executable generation - x86_64
#---------------------------------------------------------------------------------
X86_64_DIR	:= $(APK_DIR)/lib/x86_64
X86_64_LIB	:= $(X86_64_DIR)/libclassicube.so
$(X86_64_LIB): | $(X86_64_DIR)

BUILD_ARCH  := x86_64
TARGET		:= $(X86_64_LIB)
CC			:= $(TCHAIN_ROOT)/x86_64-linux-android21-clang
LINK		:= $(TCHAIN_ROOT)/x86_64-linux-android21-clang
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# Java targets
#---------------------------------------------------------------------------------
OBJ_DIR		:= $(APK_DIR)/obj
JAVA_DIR	:= misc/android/app/src/main/java/com/classicube
CLASSES_DEX	:= $(APK_DIR)/classes.dex

# note that java files can generate multiple class files (due to inner classes)
$(CLASSES_DEX): $(OBJ_DIR)
	javac $(JAVA_DIR)/*.java -d $(OBJ_DIR) -classpath $(SDK_ROOT)/android.jar --release 8
	$(TOOLS_ROOT)/dx --dex --output=$@ $(OBJ_DIR)


#---------------------------------------------------------------------------------
# misc targets
#---------------------------------------------------------------------------------
$(ARM_v5_DIR) $(ARM_32_DIR) $(ARM_64_DIR) $(X86_32_DIR) $(X86_64_DIR) $(OBJ_DIR):
	mkdir -p $@


#---------------------------------------------------------------------------------
# executable generation
#---------------------------------------------------------------------------------
MANIFEST := misc/android/app/src/main/AndroidManifest.xml
RES_DIR	 := misc/android/app/src/main/res

# create initial .apk with packaged version of resources, then add all required files
# TODO just use package ??
$(UNSIGNED_APK): $(ARM_v5_LIB) $(ARM_32_LIB) $(ARM_64_LIB) $(X86_32_LIB) $(X86_64_LIB)
	$(TOOLS_ROOT)/aapt package -f -M $(MANIFEST) -S $(RES_DIR) -F $@ -I $(SDK_ROOT)/android.jar
	$(TOOLS_ROOT)/aapt add -f $@ $^

# sign the apk with debug key (https://stackoverflow.com/questions/16711233/)
# Note per https://developer.android.com/tools/zipalign
# - if using apksigner, zipalign must be called before apksigner
# - if using jarsigner, zipalign must be called after jarsigner
$(SIGNED_APK): $(UNSIGNED_APK)
	$(TOOLS_ROOT)/zipalign -f 4 $< $@
	$(TOOLS_ROOT)/apksigner sign --ks debug.keystore --ks-pass pass:android $@ || { rm -f $@; exit 1; }
# add rm to delete cc-signed.apk in case of signing failure, so no partially generated APK is left behind

# old v1 only version (doesn't work properly now)
#cp cc-unsigned.apk cc-signed.apk
#jarsigner -verbose -sigalg SHA1withRSA -digestalg SHA1 -keystore debug.keystore -storepass android -keypass android cc-signed.apk androiddebugkey
# jarsigner -verbose
# create aligned .apk file
#$TOOLS_ROOT/zipalign -f 4 cc-signed.apk cc-final.apk
#TODO add java files to GEN_FILES


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk
