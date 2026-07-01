#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS := src third_party/bearssl
BUILD_DIR	:= build/macos

CFLAGS	:= -fvisibility=hidden -fno-ident
LDFLAGS	:= -rdynamic
LIBS 	:= -framework Security -framework Cocoa -framework OpenGL -framework IOKit -lobjc
include misc/makefiles/common_config.mk


OBJECTS := $(BUILD_DIR)/src/Window_cocoa.o

# macOS app bundle
$(TARGET).app : $(TARGET)
	mkdir -p $(TARGET)/Contents/MacOS
	mkdir -p $(TARGET)/Contents/Resources
	cp $(ENAME) $(TARGET)/Contents/MacOS/$(ENAME)
	cp misc/macOS/Info.plist   $(TARGET)/Contents/Info.plist
	cp misc/macOS/appicon.icns $(TARGET)/Contents/Resources/appicon.icns

include misc/makefiles/common_build.mk
