TARGET_NAME	:= ClassiCube
SOURCE_DIRS	:= src/android
BUILD_DIR	:= build/android

APK_DIR		:= $(BUILD_DIR)/apk
OBJ_DIR		:= $(APK_DIR)/obj

ARM_v5_PATH	:= $(APK_DIR)/lib/armeabi
ARM_32_PATH	:= $(APK_DIR)/lib/armeabi-v7a
ARM_64_PATH	:= $(APK_DIR)/lib/arm64-v8a
X86_32_PATH	:= $(APK_DIR)/lib/x86
X86_64_PATH	:= $(APK_DIR)/lib/x86_64

CLASSES_DEX	:= $(APK_DIR)/classes.dex
SIGNED_APK	:= cc-signed.apk
UNSIGNED_APK:= cc-unsigned.apk

default: $(SIGNED_APK)


#---------------------------------------------------------------------------------
# Toolchain paths
#---------------------------------------------------------------------------------
NDK_ROOT	?= /opt/android-sdk-linux/ndk/25.2.9519653/toolchains/llvm/prebuilt/linux-x86_64/bin
TOOLS_ROOT	?= $(PWD)/build-tools
SDK_ROOT	?= /opt/android-sdk-linux/platforms/android-34


#---------------------------------------------------------------------------------
# Code generation
#--------------------------------------------------------------------------------
CFLAGS	:= -fPIC -shared -fvisibility=hidden -rdynamic -funwind-tables \
			-O2 -s -fno-stack-protector -fno-math-errno -fno-ident -Werror
LDFLAGS	:= -fPIC -shared -fvisibility=hidden -rdynamic -funwind-tables \
			-O2 -s -fno-stack-protector -fno-math-errno -fno-ident -Werror
LIBS	:= -lGLESv2 -lEGL -lm -llog


#---------------------------------------------------------------------------------
# executable generation - ARMv5
#---------------------------------------------------------------------------------
BUILD_ARCH  := armv5
TARGET		:= $(ARM_v5_PATH)/libclassicube.so
CC			:= $(NDK_ROOT)/armv7a-linux-androideabi19-clang -march=armv5 -rtlib=libgcc
LINK		:= $(NDK_ROOT)/armv7a-linux-androideabi19-clang -rtlib=libgcc -L $(TOOLS_ROOT)/runtime
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# executable generation - ARM32
#---------------------------------------------------------------------------------
BUILD_ARCH  := arm32
TARGET		:= $(ARM_32_PATH)/libclassicube.so
CC			:= $(NDK_ROOT)/armv7a-linux-androideabi19-clang -mfpu=vfp3-d16
LINK		:= $(NDK_ROOT)/armv7a-linux-androideabi19-clang
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# executable generation - ARM64
#---------------------------------------------------------------------------------
BUILD_ARCH  := arm64
TARGET		:= $(ARM_64_PATH)/libclassicube.so
CC			:= $(NDK_ROOT)/aarch64-linux-android21-clang
LINK		:= $(NDK_ROOT)/aarch64-linux-android21-clang
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# executable generation - x86_32
#---------------------------------------------------------------------------------
BUILD_ARCH  := x86_32
TARGET		:= $(X86_32_PATH)/libclassicube.so
CC			:= $(NDK_ROOT)/i686-linux-android21-clang
LINK		:= $(NDK_ROOT)/i686-linux-android21-clang
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# executable generation - x86_64
#---------------------------------------------------------------------------------
BUILD_ARCH  := x86_64
TARGET		:= $(X86_64_PATH)/libclassicube.so
CC			:= $(NDK_ROOT)/x86_64-linux-android21-clang
LINK		:= $(NDK_ROOT)/x86_64-linux-android21-clang
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# executable generation
#---------------------------------------------------------------------------------
$(TARGET_NAME).apk: $(BUILD_DIR)/cc-arm32 $(BUILD_DIR)/cc-arm64 $(BUILD_DIR)/cc-x86_32 $(BUILD_DIR)/cc-x86_64
	echo "AAAAA"


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk
