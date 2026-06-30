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

ifeq ($(PLAT),darwin)
	OBJECTS += $(BUILD_DIR)/src/Window_cocoa.o
	LIBS    =
	LDFLAGS =  -rdynamic -framework Security -framework Cocoa -framework OpenGL -framework IOKit -lobjc
	BUILD_DIR = build/macos
	TARGET  = $(ENAME).app
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

# Shortcuts for default platform
sdl2:
	$(MAKE) $(PLAT) BUILD_SDL2=1
sdl3:
	$(MAKE) $(PLAT) BUILD_SDL3=1
terminal:
	$(MAKE) $(PLAT) BUILD_TERMINAL=1
release:
	$(MAKE) $(PLAT) RELEASE=1

# Build for the specified platform
#   "$(filter-out $@, $(MAKECMDGOALS))" is used to get all goals except the current one
# that way, e.g. "make freebsd clean" invokes freebsd makefile with 'clean' goal
web:
	$(MAKE) $(TARGET) PLAT=web
linux:
	$(MAKE) -f misc/makefiles/Makefile_linux.mk $(filter-out $@, $(MAKECMDGOALS))
mingw:
	$(MAKE) $(TARGET) PLAT=mingw
sunos:
	$(MAKE) -f misc/makefiles/Makefile_solaris.mk $(filter-out $@, $(MAKECMDGOALS))
hp-ux:
	$(MAKE) -f misc/makefiles/Makefile_hpux.mk $(filter-out $@, $(MAKECMDGOALS))
darwin:
	$(MAKE) $(TARGET) PLAT=darwin
freebsd:
	$(MAKE) -f misc/makefiles/Makefile_freebsd.mk $(filter-out $@, $(MAKECMDGOALS))
openbsd:
	$(MAKE) -f misc/makefiles/Makefile_openbsd.mk $(filter-out $@, $(MAKECMDGOALS))
netbsd:
	$(MAKE) -f misc/makefiles/Makefile_netbsd.mk  $(filter-out $@, $(MAKECMDGOALS))
dragonfly:
	$(MAKE) -f misc/makefiles/Makefile_flybsd.mk  $(filter-out $@, $(MAKECMDGOALS))
haiku:
	$(MAKE) -f misc/makefiles/Makefile_haiku.mk  $(filter-out $@, $(MAKECMDGOALS))
beos:
	$(MAKE) $(TARGET) PLAT=beos
serenityos:
	$(MAKE) -f misc/makefiles/Makefile_serenityos.mk $(filter-out $@, $(MAKECMDGOALS)) 
irix:
	$(MAKE) -f misc/makefiles/Makefile_irix.mk $(filter-out $@, $(MAKECMDGOALS))   
riscos:
	$(MAKE) -f misc/makefiles/Makefile_riscos.mk $(filter-out $@, $(MAKECMDGOALS))   
gnu:
	$(MAKE) -f misc/makefiles/Makefile_gnuhurd.mk $(filter-out $@, $(MAKECMDGOALS))

# Mobile systems
ios:
	$(MAKE) -f misc/ios/Makefile $(filter-out $@, $(MAKECMDGOALS))
android:
	$(MAKE) -f misc/android/Makefile $(filter-out $@, $(MAKECMDGOALS))

# Embedded systems
wince:
	$(MAKE) -f misc/makefiles/Makefile_wince.mk $(filter-out $@, $(MAKECMDGOALS))
rpi:
	$(MAKE) -f misc/makefiles/Makefile_rpi.mk $(filter-out $@, $(MAKECMDGOALS))

# SEGA consoles
32x:
	$(MAKE) -f misc/32x/Makefile $(filter-out $@, $(MAKECMDGOALS))
saturn:
	$(MAKE) -f misc/saturn/Makefile $(filter-out $@, $(MAKECMDGOALS))
dreamcast:
	$(MAKE) -f misc/dreamcast/Makefile $(filter-out $@, $(MAKECMDGOALS))

# Sony consoles
psp:
	$(MAKE) -f misc/psp/Makefile $(filter-out $@, $(MAKECMDGOALS))
vita:
	$(MAKE) -f misc/vita/Makefile $(filter-out $@, $(MAKECMDGOALS))
ps1:
	$(MAKE) -f misc/ps1/Makefile $(filter-out $@, $(MAKECMDGOALS))
ps2:
	$(MAKE) -f misc/ps2/Makefile $(filter-out $@, $(MAKECMDGOALS))
ps3:
	$(MAKE) -f misc/ps3/Makefile $(filter-out $@, $(MAKECMDGOALS))
ps4:
	$(MAKE) -f misc/ps4/Makefile $(filter-out $@, $(MAKECMDGOALS))

# Microsoft consoles
xbox:
	$(MAKE) -f misc/xbox/Makefile $(filter-out $@, $(MAKECMDGOALS))
xbox360:
	$(MAKE) -f misc/xbox360/Makefile $(filter-out $@, $(MAKECMDGOALS))

# Nintendo consoles
n64:
	$(MAKE) -f misc/n64/Makefile $(filter-out $@, $(MAKECMDGOALS))
gba:
	$(MAKE) -f misc/gba/Makefile $(filter-out $@, $(MAKECMDGOALS))
ds:
	$(MAKE) -f misc/nds/Makefile $(filter-out $@, $(MAKECMDGOALS))
3ds:
	$(MAKE) -f misc/3ds/Makefile $(filter-out $@, $(MAKECMDGOALS))
gamecube:
	$(MAKE) -f misc/gc/Makefile $(filter-out $@, $(MAKECMDGOALS))
wii:
	$(MAKE) -f misc/wii/Makefile $(filter-out $@, $(MAKECMDGOALS))
wiiu:
	$(MAKE) -f misc/wiiu/Makefile $(filter-out $@, $(MAKECMDGOALS))
switch:
	$(MAKE) -f misc/switch/Makefile $(filter-out $@, $(MAKECMDGOALS))

# Other systems
os/2:
	$(MAKE) -f misc/os2/Makefile $(filter-out $@, $(MAKECMDGOALS))
dos:
	$(MAKE) -f misc/msdos/Makefile $(filter-out $@, $(MAKECMDGOALS))
macclassic_68k:
	$(MAKE) -f misc/macclassic/Makefile_68k.mk $(filter-out $@, $(MAKECMDGOALS))
macclassic_ppc:
	$(MAKE) -f misc/macclassic/Makefile_ppc.mk $(filter-out $@, $(MAKECMDGOALS))
amiga_gcc:
	$(MAKE) -f misc/amiga/Makefile_68k $(filter-out $@, $(MAKECMDGOALS))
amiga:
	$(MAKE) -f misc/amiga/Makefile $(filter-out $@, $(MAKECMDGOALS))
atari_st:
	$(MAKE) -f misc/atari_st/Makefile $(filter-out $@, $(MAKECMDGOALS))

# Cleans up all build .o files (except when clean goal is from e.g 'make freebsd clean')
ifeq ($(MAKECMDGOALS),clean)
clean:
	$(RM) $(OBJECTS)
else
clean:
	@echo "NOTE: Skipping 'clean' due to not being the only goal (all goals: $(MAKECMDGOALS))"
endif


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
