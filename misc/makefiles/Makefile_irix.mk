#-----------------------------
# Configurable flags and names
#-----------------------------
SOURCE_DIRS = src third_party/bearssl
BUILD_DIR	= build/irix

# Name of the main executable
TARGET  = ClassiCube
# Flags passed to the C compiler
CFLAGS	= 
# Flags passed to the linker
LDFLAGS	= -g -rdynamic
# Libraries to link against
LIBS 	= -lGL -lX11 -lXi -lpthread -ldl

CC      = gcc


include misc/makefiles/Makefile_common.mk
