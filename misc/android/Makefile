.SUFFIXES:


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
# Name of the final output
TARGET 			= ClassiCube
# List of directories containing source code
SOURCE_DIRS		= src src/ios third_party/bearssl


#---------------------------------------------------------------------------------
# Paths
#---------------------------------------------------------------------------------
NDK_ROOT	?= /opt/android-sdk-linux/ndk/25.2.9519653/toolchains/llvm/prebuilt/linux-x86_64/bin
TOOLS_ROOT	?= $(PWD)/build-tools
SDK_ROOT	?= /opt/android-sdk-linux/platforms/android-34

# Directories where object files are placed
BUILD_DIR_ROOT	:= build/android
BUILD_DIR_A16	:= $(BUILD_DIR_ROOT)/arm16
BUILD_DIR_A32	:= $(BUILD_DIR_ROOT)/arm32
BUILD_DIR_A64	:= $(BUILD_DIR_ROOT)/arm64
BUILD_DIR_X32	:= $(BUILD_DIR_ROOT)/x86_32
BUILD_DIR_X64	:= $(BUILD_DIR_ROOT)/x86_64


#---------------------------------------------------------------------------------
# Compiler tools
#---------------------------------------------------------------------------------
ARM32_CC	:= $(NDK_ROOT)/armv7a-linux-androideabi19-clang
ARM64_CC	:= $(NDK_ROOT)/aarch64-linux-android21-clang
X86_32_CC	:= $(NDK_ROOT)/i686-linux-android21-clang
X86_64_CC	:= $(NDK_ROOT)/x86_64-linux-android21-clang

