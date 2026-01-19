SOURCE_DIR  = src
BUILD_DIR   = build
C_SOURCES   = $(wildcard $(SOURCE_DIR)/*.c)
OBJECTS   	= $(patsubst %.c, $(BUILD_DIR)/%.o, $(C_SOURCES))
BUILD_DIRS	= $(BUILD_DIR) $(BUILD_DIR)/src

##############################
# Configurable flags and names
##############################
# Flags passed to the C compiler
CFLAGS  = -pipe -fno-math-errno -Werror -Wno-error=missing-braces -Wno-error=strict-aliasing
# Flags passed to the linker
LDFLAGS = -g -rdynamic
# Name of the main executable
ENAME   = ClassiCube
# Name of the final target file
# (usually this is the executable, but e.g. is the app bundle on macOS)
TARGET  := $(ENAME)

# Enables dependency tracking (https://make.mad-scientist.net/papers/advanced-auto-dependency-generation/)
# This ensures that changing a .h file automatically results in the .c files using it being auto recompiled when next running make
# On older systems the required GCC options may not be supported - in which case just change TRACK_DEPENDENCIES to 0
TRACK_DEPENDENCIES=1
# link using C Compiler by default
LINK = $(CC)
# Whether to add BearSSL source files to list of files to compile
BEARSSL=1
# Optimization level in release builds
OPT_LEVEL=1


#################################################################
# Determine shell command used to remove files (for "make clean")
#################################################################
ifndef RM
	# No prefined RM variable, try to guess OS default
	ifeq ($(OS),Windows_NT)
		RM = del
	else
		RM = rm -f
	endif
endif


###########################################################
# If target platform isn't specified, default to current OS
###########################################################
ifndef $(PLAT)
	ifeq ($(OS),Windows_NT)
		PLAT = mingw
	else
		PLAT = $(shell uname -s | tr '[:upper:]' '[:lower:]')
	endif
endif


#########################################################
# Setup environment appropriate for the specific platform
#########################################################
ifeq ($(PLAT),web)
	CC      = emcc
	OEXT    = .html
	CFLAGS  = -g
	LDFLAGS = -g -s WASM=1 -s NO_EXIT_RUNTIME=1 -s ABORTING_MALLOC=0 -s ALLOW_MEMORY_GROWTH=1 -s TOTAL_STACK=256Kb --js-library $(SOURCE_DIR)/webclient/interop_web.js
	BUILD_DIR = build/web
	BEARSSL = 0

	BUILD_DIRS += $(BUILD_DIR)/src/webclient
	C_SOURCES  += $(wildcard src/webclient/*.c)
endif

ifeq ($(PLAT),mingw)
	CC      =  gcc
	OEXT    =  .exe
	CFLAGS  += -DUNICODE
	LDFLAGS =  -g
	LIBS    =  -mwindows -lwinmm
	BUILD_DIR = build/win
endif

ifeq ($(PLAT),linux)
	# -lm may be needed for __builtin_sqrtf (in cases where it isn't replaced by a CPU instruction intrinsic)
	LIBS    =  -lX11 -lXi -lpthread -lGL -ldl -lm
	BUILD_DIR = build/linux

	# Detect MCST LCC, where -O3 is about equivalent to -O1
	ifeq ($(shell $(CC) -dM -E -xc - < /dev/null | grep -o __MCST__),__MCST__)
		OPT_LEVEL=3
	endif
endif

ifeq ($(PLAT),sunos)
	LIBS    =  -lsocket -lX11 -lXi -lGL
	BUILD_DIR = build/solaris
endif

ifeq ($(PLAT),hp-ux)
	CC      = gcc
	CFLAGS  += -std=c99 -D_POSIX_C_SOURCE=200112L -D_XOPEN_SOURCE=600 -D_DEFAULT_SOURCE -D_BSD_SOURCE
	LDFLAGS =
	LIBS    = -lm -lX11 -lXi -lXext -L/opt/graphics/OpenGL/lib/hpux32 -lGL -lpthread
	BUILD_DIR = build/hpux
endif

ifeq ($(PLAT),darwin)
	OBJECTS += $(BUILD_DIR)/src/Window_cocoa.o
	LIBS    =
	LDFLAGS =  -rdynamic -framework Security -framework Cocoa -framework OpenGL -framework IOKit -lobjc
	BUILD_DIR = build/macos
	TARGET  = $(ENAME).app
endif

ifeq ($(PLAT),freebsd)
	CFLAGS  += -I /usr/local/include
	LDFLAGS =  -L /usr/local/lib -rdynamic
	LIBS    =  -lexecinfo -lGL -lX11 -lXi -lpthread
	BUILD_DIR = build/freebsd
endif

ifeq ($(PLAT),openbsd)
	CFLAGS  += -I /usr/X11R6/include -I /usr/local/include
	LDFLAGS =  -L /usr/X11R6/lib -L /usr/local/lib -rdynamic
	LIBS    =  -lexecinfo -lGL -lX11 -lXi -lpthread
	BUILD_DIR = build/openbsd
endif

ifeq ($(PLAT),netbsd)
	CFLAGS  += -I /usr/X11R7/include -I /usr/pkg/include
	LDFLAGS =  -L /usr/X11R7/lib -L /usr/pkg/lib -rdynamic -Wl,-R/usr/X11R7/lib
	LIBS    =  -lexecinfo -lGL -lX11 -lXi -lpthread
	BUILD_DIR = build/netbsd
endif

ifeq ($(PLAT),dragonfly)
	CFLAGS  += -I /usr/local/include
	LDFLAGS =  -L /usr/local/lib -rdynamic
	LIBS    =  -lexecinfo -lGL -lX11 -lXi -lpthread
	BUILD_DIR = build/flybsd
endif

ifeq ($(PLAT),haiku)
	OBJECTS += $(BUILD_DIR)/src/Platform_BeOS.o $(BUILD_DIR)/src/Window_BeOS.o
	CFLAGS  = -pipe -fno-math-errno
	LDFLAGS = -g
	LINK    = $(CXX)
	LIBS    = -lGL -lnetwork -lbe -lgame -ltracker
	BUILD_DIR = build/haiku
endif

ifeq ($(PLAT),beos)
	OBJECTS += $(BUILD_DIR)/src/Platform_BeOS.o $(BUILD_DIR)/src/Window_BeOS.o
	CFLAGS  = -pipe
	LDFLAGS = -g
	LINK    = $(CXX)
	LIBS    = -lGL -lnetwork -lbe -lgame -ltracker
	BUILD_DIR = build/beos
	TRACK_DEPENDENCIES = 0
	BEARSSL = 0
endif

ifeq ($(PLAT),serenityos)
	LIBS    = -lgl -lSDL2
	BUILD_DIR = build/serenity
endif

ifeq ($(PLAT),irix)
	CC      = gcc
	LIBS    = -lGL -lX11 -lXi -lpthread -ldl
	BUILD_DIR = build/irix
endif

ifeq ($(PLAT),rpi)
	CFLAGS += -DCC_BUILD_RPI
	LIBS    =  -lpthread -lX11 -lXi -lEGL -lGLESv2 -ldl
	BUILD_DIR = build/rpi
endif

ifeq ($(PLAT),riscos)
	LIBS    =
	LDFLAGS = -g
	BUILD_DIR = build/riscos
endif

ifeq ($(PLAT),wince)
	CC      =  arm-mingw32ce-gcc
	OEXT    =  .exe
	CFLAGS  += -march=armv5te -DUNICODE -D_WIN32_WCE -std=gnu99
	LDFLAGS =  -g
	LIBS    =  -lcoredll -lws2
	BUILD_DIR = build/wince
endif

ifeq ($(PLAT),os/2)
	BUILD_DIR =	build/os2
endif

ifdef BUILD_SDL2
	CFLAGS += -DCC_WIN_BACKEND=CC_WIN_BACKEND_SDL2
	LIBS += -lSDL2
endif
ifdef BUILD_SDL3
	CFLAGS += -DCC_WIN_BACKEND=CC_WIN_BACKEND_SDL3
	LIBS += -lSDL3
endif
ifdef BUILD_TERMINAL
	CFLAGS += -DCC_WIN_BACKEND=CC_WIN_BACKEND_TERMINAL -DCC_GFX_BACKEND=CC_GFX_BACKEND_SOFTGPU
	LIBS := $(subst mwindows,mconsole,$(LIBS))
endif

ifeq ($(BEARSSL),1)
	BUILD_DIRS += $(BUILD_DIR)/third_party/bearssl
	C_SOURCES  += $(wildcard third_party/bearssl/*.c)
endif

ifdef RELEASE
	CFLAGS += -O$(OPT_LEVEL)
else
	CFLAGS += -g
endif

default: $(PLAT)

# Build for the specified platform
web:
	$(MAKE) $(TARGET) PLAT=web
linux:
	$(MAKE) $(TARGET) PLAT=linux
mingw:
	$(MAKE) $(TARGET) PLAT=mingw
sunos:
	$(MAKE) $(TARGET) PLAT=sunos
hp-ux:
	$(MAKE) $(TARGET) PLAT=hp-ux
darwin:
	$(MAKE) $(TARGET) PLAT=darwin
freebsd:
	$(MAKE) $(TARGET) PLAT=freebsd
openbsd:
	$(MAKE) $(TARGET) PLAT=openbsd
netbsd:
	$(MAKE) $(TARGET) PLAT=netbsd
dragonfly:
	$(MAKE) $(TARGET) PLAT=dragonfly
haiku:
	$(MAKE) $(TARGET) PLAT=haiku
beos:
	$(MAKE) $(TARGET) PLAT=beos
serenityos:
	$(MAKE) $(TARGET) PLAT=serenityos
irix:
	$(MAKE) $(TARGET) PLAT=irix
riscos:
	$(MAKE) $(TARGET) PLAT=riscos    
wince:
	$(MAKE) $(TARGET) PLAT=wince
# Default overrides
sdl2:
	$(MAKE) $(TARGET) BUILD_SDL2=1
sdl3:
	$(MAKE) $(TARGET) BUILD_SDL3=1
terminal:
	$(MAKE) $(TARGET) BUILD_TERMINAL=1
release:
	$(MAKE) $(TARGET) RELEASE=1

# Some builds require more complex handling, so are moved to
#  separate makefiles to avoid having one giant messy makefile
32x:
	$(MAKE) -f misc/32x/Makefile
saturn:
	$(MAKE) -f misc/saturn/Makefile
dreamcast:
	$(MAKE) -f misc/dreamcast/Makefile
psp:
	$(MAKE) -f misc/psp/Makefile
vita:
	$(MAKE) -f misc/vita/Makefile
ps1:
	$(MAKE) -f misc/ps1/Makefile
ps2:
	$(MAKE) -f misc/ps2/Makefile
ps3:
	$(MAKE) -f misc/ps3/Makefile
ps4:
	$(MAKE) -f misc/ps4/Makefile
xbox:
	$(MAKE) -f misc/xbox/Makefile
xbox360:
	$(MAKE) -f misc/xbox360/Makefile
n64:
	$(MAKE) -f misc/n64/Makefile
gba:
	$(MAKE) -f misc/gba/Makefile
ds:
	$(MAKE) -f misc/nds/Makefile
3ds:
	$(MAKE) -f misc/3ds/Makefile
gamecube:
	$(MAKE) -f misc/gc/Makefile
wii:
	$(MAKE) -f misc/wii/Makefile
wiiu:
	$(MAKE) -f misc/wiiu/Makefile
switch:
	$(MAKE) -f misc/switch/Makefile

os/2:
	$(MAKE) -f misc/os2/Makefile
dos:
	$(MAKE) -f misc/msdos/Makefile
macclassic_68k:
	$(MAKE) -f misc/macclassic/Makefile_68k
macclassic_ppc:
	$(MAKE) -f misc/macclassic/Makefile_ppc
amiga_gcc:
	$(MAKE) -f misc/amiga/Makefile_68k
amiga:
	$(MAKE) -f misc/amiga/Makefile
atari_st:
	$(MAKE) -f misc/atari_st/Makefile
ios:
	$(MAKE) -f misc/ios/Makefile

# Cleans up all build .o files
clean:
	$(RM) $(OBJECTS)


#################################################
# Source files and executable compilation section
#################################################
# Auto creates directories for build files (.o and .d files)
$(BUILD_DIRS):
	mkdir -p $@

# Main executable (typically just 'ClassiCube' or 'ClassiCube.exe')
$(ENAME): $(BUILD_DIRS) $(OBJECTS)
	$(LINK) $(LDFLAGS) -o $@$(OEXT) $(OBJECTS) $(EXTRA_LIBS) $(LIBS)
	@echo "----------------------------------------------------"
	@echo "Successfully compiled executable file: $(ENAME)"
	@echo "----------------------------------------------------"

# macOS app bundle
$(ENAME).app : $(ENAME)
	mkdir -p $(TARGET)/Contents/MacOS
	mkdir -p $(TARGET)/Contents/Resources
	cp $(ENAME) $(TARGET)/Contents/MacOS/$(ENAME)
	cp misc/macOS/Info.plist   $(TARGET)/Contents/Info.plist
	cp misc/macOS/appicon.icns $(TARGET)/Contents/Resources/appicon.icns


# === Compiling with dependency tracking ===
# NOTE: Tracking dependencies might not work on older systems - disable this if so
ifeq ($(TRACK_DEPENDENCIES), 1)

DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d
DEPFILES := $(patsubst %.o, %.d, $(OBJECTS))
$(DEPFILES):

$(BUILD_DIR)/%.o : %.c $(BUILD_DIR)/%.d
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(DEPFLAGS) -c $< -o $@
$(BUILD_DIR)/%.o : %.cpp $(BUILD_DIR)/%.d
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(DEPFLAGS) -c $< -o $@
$(BUILD_DIR)/%.o : %.m $(BUILD_DIR)/%.d
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(DEPFLAGS) -c $< -o $@

include $(wildcard $(DEPFILES))
# === Compiling WITHOUT dependency tracking ===
else

$(BUILD_DIR)/%.o : %.c
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@
$(BUILD_DIR)/%.o : %.cpp
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@
endif

# EXTRA_CFLAGS and EXTRA_LIBS are not defined in the makefile intentionally -
# define them on the command line as a simple way of adding CFLAGS/LIBS
