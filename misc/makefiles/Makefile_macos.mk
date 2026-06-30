#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS = src third_party/bearssl
BUILD_DIR	= build/macos

# Name of the main executable
TARGET  = ClassiCube
# Flags passed to the C compiler
CFLAGS	=
# Flags passed to the linker
LDFLAGS	= -rdynamic -framework Security -framework Cocoa -framework OpenGL -framework IOKit -lobjc
# Libraries to link against
LIBS 	=

OBJECTS = $(BUILD_DIR)/src/Window_cocoa.o


# macOS app bundle
$(TARGET).app : $(TARGET)
	mkdir -p $(TARGET)/Contents/MacOS
	mkdir -p $(TARGET)/Contents/Resources
	cp $(ENAME) $(TARGET)/Contents/MacOS/$(ENAME)
	cp misc/macOS/Info.plist   $(TARGET)/Contents/Info.plist
	cp misc/macOS/appicon.icns $(TARGET)/Contents/Resources/appicon.icns


include misc/makefiles/Makefile_common.mk
