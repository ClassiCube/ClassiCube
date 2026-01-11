.SUFFIXES:


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
# Name of the final output
TARGET 			= ClassiCube
# List of directories containing source code
SOURCE_DIRS		= src src/ios third_party/bearssl

# CPU architecture names
ARCH_32	?= armv7
ARCH_64	?= arm64

# Directory where object files are placed
BUILD_DIR_APP	:= build/ios/$(TARGET).app
BUILD_DIR_32	:= build/ios/$(ARCH_32)
BUILD_DIR_64	:= build/ios/$(ARCH_64)

IOS_SDK_PATH	?= /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS12.1.sdk
IOS_MIN_VERSION ?= 6.0


#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------
M_FILES := $(wildcard src/ios/*.m)
C_FILES := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
OBJS_32	:= $(addprefix $(BUILD_DIR_32)/, $(notdir $(C_FILES:%.c=%.o) $(M_FILES:%.m=%.o)))
OBJS_64	:= $(addprefix $(BUILD_DIR_64)/, $(notdir $(C_FILES:%.c=%.o) $(M_FILES:%.m=%.o)))

CFLAGS  := -g -Os -fno-math-errno -fpascal-strings -fvisibility=hidden -fno-common -Wno-missing-field-initializers -Wno-missing-prototypes -Werror=return-type -Wunreachable-code -Wno-missing-braces -Wparentheses -Wswitch -Wunused-function -Wno-unused-label -Wno-unused-parameter -Wunused-variable -Wunused-value -Wuninitialized -Wconditional-uninitialized -Wno-shadow -Wno-conversion -Wconstant-conversion -Wint-conversion -Wbool-conversion -Wenum-conversion -Wno-float-conversion -Wobjc-literal-conversion -Wshorten-64-to-32 -Wpointer-sign -isysroot $(IOS_SDK_PATH) -fstrict-aliasing -miphoneos-version-min=$(IOS_MIN_VERSION) -Wno-sign-conversion -Winfinite-recursion -Wcomma -Wblock-capture-autoreleasing -Wstrict-prototypes
MFLAGS  := $(CFLAGS)

LDFLAGS  := -isysroot $(IOS_SDK_PATH) -Xlinker -rpath -Xlinker @executable_path/Frameworks
LIBS 	 := -framework OpenGLES -framework UIKit -framework Security -framework CoreGraphics -framework CoreText -framework QuartzCore -lobjc

# Dependency tracking
DEPFLAGS_32 := -MT $@ -MMD -MP -MF $(BUILD_DIR_32)/$*.d
DEPFLAGS_64 := -MT $@ -MMD -MP -MF $(BUILD_DIR_64)/$*.d
DEPFILES_32 := $(OBJS_32:%.o=%.d)
DEPFILES_64 := $(OBJS_64:%.o=%.d)


#---------------------------------------------------------------------------------
# Compiler tools
#---------------------------------------------------------------------------------
CC 		:= /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
LD 		:= /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
LIPO	:= /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/lipo
ACTOOL	:= /Applications/Xcode.app/Contents/Developer/usr/bin/actool

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all: $(BUILD_DIR_32) $(BUILD_DIR_64) $(BUILD_DIR_APP) $(TARGET).ipa

$(BUILD_DIR_32):
	mkdir -p $(BUILD_DIR_32)

$(BUILD_DIR_64):
	mkdir -p $(BUILD_DIR_64)

$(BUILD_DIR_APP):
	mkdir -p $(BUILD_DIR_APP)


#---------------------------------------------------------------------------------
# executable generation
#---------------------------------------------------------------------------------
$(BUILD_DIR_32)/$(TARGET): $(OBJS_32)
	export IPHONEOS_DEPLOYMENT_TARGET=$(IOS_MIN_VERSION)
	$(LD) $^ -arch $(ARCH_32) $(LDFLAGS) $(LIBS) -miphoneos-version-min=$(IOS_MIN_VERSION) -dead_strip -fobjc-link-runtime -o $@

$(BUILD_DIR_64)/$(TARGET): $(OBJS_64)
	export IPHONEOS_DEPLOYMENT_TARGET=$(IOS_MIN_VERSION)
	$(LD) $^ -arch $(ARCH_64) $(LDFLAGS) $(LIBS) -miphoneos-version-min=$(IOS_MIN_VERSION) -dead_strip -fobjc-link-runtime -o $@


$(BUILD_DIR_APP)/$(TARGET): $(BUILD_DIR_32)/$(TARGET) $(BUILD_DIR_64)/$(TARGET)
	$(LIPO) -create $^ -output $@

$(BUILD_DIR_APP)/Assets.car:
	$(ACTOOL) --output-format human-readable-text --notices --warnings --output-partial-info-plist $(BUILD_DIR_APP)/assetcatalog_generated_info.plist --app-icon AppIcon --compress-pngs --enable-on-demand-resources YES --target-device iphone --target-device ipad --minimum-deployment-target $(IOS_MIN_VERSION) --platform iphoneos --product-type com.apple.product-type.application --compile $(BUILD_DIR_APP) misc/ios/ClassiCube/Assets.xcassets

$(BUILD_DIR_APP)/Info.plist: misc/ios/Info.plist
	cp $^ $@

$(BUILD_DIR_APP)/PkgInfo: misc/ios/PkgInfo
	cp $^ $@


$(TARGET).ipa: $(BUILD_DIR_APP)/$(TARGET) $(BUILD_DIR_APP)/Assets.car $(BUILD_DIR_APP)/Info.plist $(BUILD_DIR_APP)/PkgInfo
	rm -rf Payload
	mkdir -p Payload/$(TARGET).app
	cp -R $(BUILD_DIR_APP) Payload/$(TARGET).app
	zip -r $(TARGET).ipa Payload


#---------------------------------------------------------------------------------
# 32 bit object generation
#---------------------------------------------------------------------------------
$(BUILD_DIR_32)/%.o : src/%.c
	$(CC) -arch $(ARCH_32) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR_32)/%.o : src/ios/%.c
	$(CC) -arch $(ARCH_32) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR_32)/%.o : src/ios/%.m
	$(CC) -arch $(ARCH_32) $(CFLAGS) $(DEPFLAGS) -c $< -o $@
	
$(BUILD_DIR_32)/%.o : third_party/bearssl/%.c
	$(CC) -arch $(ARCH_32) $(CFLAGS) -c $< -o $@


#---------------------------------------------------------------------------------
# 64 bit object generation
#---------------------------------------------------------------------------------
$(BUILD_DIR_64)/%.o : src/%.c
	$(CC) -arch $(ARCH_64) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR_64)/%.o : src/ios/%.c
	$(CC) -arch $(ARCH_64) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR_64)/%.o : src/ios/%.m
	$(CC) -arch $(ARCH_64) $(CFLAGS) $(DEPFLAGS) -c $< -o $@
	
$(BUILD_DIR_64)/%.o : third_party/bearssl/%.c
	$(CC) -arch $(ARCH_64) $(CFLAGS) -c $< -o $@


#---------------------------------------------------------------------------------
# Dependency tracking
#---------------------------------------------------------------------------------
$(DEPFILES_32):
$(DEPFILES_64):

include $(wildcard $(DEPFILES_32))
include $(wildcard $(DEPFILES_64))
