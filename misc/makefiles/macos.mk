#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS := src src/macos third_party/bearssl
BUILD_DIR	:= build/macos

CFLAGS	:= -fvisibility=hidden -fno-ident -Wno-error=deprecated-declarations
LDFLAGS	:= -rdynamic
LIBS 	:= -framework Security -framework Cocoa -framework OpenGL -framework IOKit -lobjc
include misc/makefiles/common_config.mk

# macOS app bundle
$(TARGET).app : $(TARGET)
	mkdir -p $(TARGET)/Contents/MacOS
	mkdir -p $(TARGET)/Contents/Resources
	cp $(ENAME) $(TARGET)/Contents/MacOS/$(ENAME)
	cp misc/macOS/Info.plist   $(TARGET)/Contents/Info.plist
	cp misc/macOS/appicon.icns $(TARGET)/Contents/Resources/appicon.icns

include misc/makefiles/common_build.mk
