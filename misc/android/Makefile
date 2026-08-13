TARGET_NAME	:= ClassiCube
SOURCE_DIRS	:= src src/android third_party/bearssl
BUILD_DIR	:= build/android


#---------------------------------------------------------------------------------
# Paths
#---------------------------------------------------------------------------------
NDK_ROOT	?= /opt/android-sdk-linux/ndk/25.2.9519653/toolchains/llvm/prebuilt/linux-x86_64/bin
TOOLS_ROOT	?= $(PWD)/build-tools
SDK_ROOT	?= /opt/android-sdk-linux/platforms/android-34

#---------------------------------------------------------------------------------
# executable generation
#---------------------------------------------------------------------------------
$(TARGET_NAME).apk: $(BUILD_DIR)/cc-arm32 $(BUILD_DIR)/cc-arm64 $(BUILD_DIR)/cc-x86_32 $(BUILD_DIR)/cc-x86_64
	echo "AAAAA"


#---------------------------------------------------------------------------------
# executable generation - ARM32
#---------------------------------------------------------------------------------
BUILD_ARCH  := arm32
TARGET		:= $(BUILD_DIR)/cc-$(BUILD_ARCH)
CC			:= $(NDK_ROOT)/armv7a-linux-androideabi19-clang
LINK		:= $(CC)
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# executable generation - ARM64
#---------------------------------------------------------------------------------
BUILD_ARCH  := arm64
TARGET		:= $(BUILD_DIR)/cc-$(BUILD_ARCH)
CC			:= $(NDK_ROOT)/aarch64-linux-android21-clang
LINK		:= $(CC)
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# executable generation - x86_32
#---------------------------------------------------------------------------------
BUILD_ARCH  := x86_32
TARGET		:= $(BUILD_DIR)/cc-$(BUILD_ARCH)
CC			:= $(NDK_ROOT)/i686-linux-android21-clang
LINK		:= $(CC)
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# executable generation - x86_64
#---------------------------------------------------------------------------------
BUILD_ARCH  := x86_64
TARGET		:= $(BUILD_DIR)/cc-$(BUILD_ARCH)
CC			:= $(NDK_ROOT)/x86_64-linux-android21-clang
LINK		:= $(CC)
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk
