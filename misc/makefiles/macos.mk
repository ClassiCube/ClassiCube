SOURCE_DIRS := src src/macos third_party/bearssl
BUILD_DIR	:= build/macos
TARGET 		:= ClassiCube

CFLAGS	:= -fvisibility=hidden -fno-ident -Wno-error=deprecated-declarations
LDFLAGS	:= -rdynamic
LIBS 	:= -framework Security -framework Cocoa -framework OpenGL -framework IOKit -lobjc
include misc/makefiles/common_config.mk


#---------------------------------------------------------------------------------
# executable generation
#---------------------------------------------------------------------------------
# macOS app bundle
$(TARGET).app : $(TARGET)
	mkdir -p $@/Contents/MacOS
	mkdir -p $@/Contents/Resources
	cp $< $@/Contents/MacOS/$<
	cp misc/macOS/Info.plist   $@/Contents/Info.plist
	cp misc/macOS/appicon.icns $@/Contents/Resources/appicon.icns

include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk
