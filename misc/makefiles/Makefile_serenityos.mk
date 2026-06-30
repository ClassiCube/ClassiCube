#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS = src third_party/bearssl
BUILD_DIR	= build/serenity

# Name of the main executable
TARGET  = ClassiCube
# Flags passed to the C compiler
CFLAGS	= 
# Flags passed to the linker
LDFLAGS	= -g -rdynamic
# Libraries to link against
LIBS 	= -lgl -lSDL2


include misc/makefiles/Makefile_common.mk
