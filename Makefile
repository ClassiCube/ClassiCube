SOURCE_DIR  := src
BUILD_DIR   := build
C_SOURCES   := $(wildcard $(SOURCE_DIR)/*.c)
C_OBJECTS   := $(patsubst $(SOURCE_DIR)/%.c, $(BUILD_DIR)/%.o, $(C_SOURCES))

OBJECTS := $(C_OBJECTS)
ENAME   = ClassiCube
DEL     = rm -f
CFLAGS  = -g -pipe -fno-math-errno
LDFLAGS = -g -rdynamic

# Enables dependency tracking (https://make.mad-scientist.net/papers/advanced-auto-dependency-generation/)
# This ensures that changing a .h file automatically results in the .c files using it being auto recompiled when next running make
# On older systems the required GCC options may not be supported - in which case just change TRACK_DEPENDENCIES to 0
TRACK_DEPENDENCIES=1

ifndef $(PLAT)
	ifeq ($(OS),Windows_NT)
		PLAT = mingw
	else
		PLAT = $(shell uname -s | tr '[:upper:]' '[:lower:]')
	endif
endif

ifeq ($(PLAT),web)
	CC      = emcc
	OEXT    = .html
	CFLAGS  = -g
	LDFLAGS = -s WASM=1 -s NO_EXIT_RUNTIME=1 -s ALLOW_MEMORY_GROWTH=1 -s TOTAL_STACK=1Mb --js-library $(SOURCE_DIR)/interop_web.js
endif

ifeq ($(PLAT),mingw)
	CC      = gcc
	OEXT    = .exe
	CFLAGS  = -g -pipe -DUNICODE -fno-math-errno
	LDFLAGS = -g
	LIBS    = -mwindows -lwinmm -limagehlp
endif

ifeq ($(PLAT),linux)
	CFLAGS  = -g -pipe -fno-math-errno -DCC_BUILD_ICON
	LIBS    = -lX11 -lXi -lpthread -lGL -ldl
endif

ifeq ($(PLAT),sunos)
	CFLAGS  = -g -pipe -fno-math-errno
	LIBS    = -lsocket -lX11 -lXi -lGL
endif

ifeq ($(PLAT),darwin)
	OBJECTS += $(BUILD_DIR)/interop_cocoa.o
	CFLAGS  = -g -pipe -fno-math-errno -DCC_BUILD_ICON
	LIBS    =
	LDFLAGS = -rdynamic -framework Cocoa -framework OpenGL -framework IOKit -lobjc
endif

ifeq ($(PLAT),freebsd)
	CFLAGS  = -g -pipe -I /usr/local/include -fno-math-errno -DCC_BUILD_ICON
	LDFLAGS = -L /usr/local/lib -rdynamic
	LIBS    = -lexecinfo -lGL -lX11 -lXi -lpthread
endif

ifeq ($(PLAT),openbsd)
	CFLAGS  = -g -pipe -I /usr/X11R6/include -I /usr/local/include -fno-math-errno -DCC_BUILD_ICON
	LDFLAGS = -L /usr/X11R6/lib -L /usr/local/lib -rdynamic
	LIBS    = -lexecinfo -lGL -lX11 -lXi -lpthread
endif

ifeq ($(PLAT),netbsd)
	CFLAGS  = -g -pipe -I /usr/X11R7/include -I /usr/pkg/include -fno-math-errno -DCC_BUILD_ICON
	LDFLAGS = -L /usr/X11R7/lib -L /usr/pkg/lib -rdynamic
	LIBS    = -lexecinfo -lGL -lX11 -lXi -lpthread
endif

ifeq ($(PLAT),dragonfly)
	CFLAGS  = -g -pipe -I /usr/local/include -fno-math-errno -DCC_BUILD_ICON
	LDFLAGS = -L /usr/local/lib -rdynamic
	LIBS    = -lexecinfo -lGL -lX11 -lXi -lpthread
endif

ifeq ($(PLAT),haiku)
	OBJECTS += $(BUILD_DIR)/interop_BeOS.o
	CFLAGS  = -g -pipe -fno-math-errno
	LDFLAGS = -g
	LIBS    = -lGL -lnetwork -lstdc++ -lbe -lgame -ltracker
endif

ifeq ($(PLAT),beos)
	OBJECTS += $(BUILD_DIR)/interop_BeOS.o
	CFLAGS  = -g -pipe
	LDFLAGS = -g
	LIBS    = -lGL -lnetwork -lstdc++ -lbe -lgame -ltracker
	TRACK_DEPENDENCIES=0
endif

ifeq ($(PLAT),serenityos)
	LIBS    = -lgl -lSDL2
endif

ifeq ($(PLAT),irix)
	CC      = gcc
	LIBS    = -lGL -lX11 -lXi -lpthread -ldl
endif

ifeq ($(OS),Windows_NT)
	DEL     = del
endif

default: $(PLAT)

web:
	$(MAKE) $(ENAME) PLAT=web
linux:
	$(MAKE) $(ENAME) PLAT=linux
mingw:
	$(MAKE) $(ENAME) PLAT=mingw
sunos:
	$(MAKE) $(ENAME) PLAT=sunos
darwin:
	$(MAKE) $(ENAME) PLAT=darwin
freebsd:
	$(MAKE) $(ENAME) PLAT=freebsd
openbsd:
	$(MAKE) $(ENAME) PLAT=openbsd
netbsd:
	$(MAKE) $(ENAME) PLAT=netbsd
dragonfly:
	$(MAKE) $(ENAME) PLAT=dragonfly
haiku:
	$(MAKE) $(ENAME) PLAT=haiku
beos:
	$(MAKE) $(ENAME) PLAT=beos
serenityos:
	$(MAKE) $(ENAME) PLAT=serenityos
irix:
	$(MAKE) $(ENAME) PLAT=irix

# Some builds require more complex handling, so are moved to
#  separate makefiles to avoid having one giant messy makefile
dreamcast:
	$(MAKE) -f misc/dreamcast/Makefile
saturn:
	$(MAKE) -f misc/saturn/Makefile
psp:
	$(MAKE) -f misc/psp/Makefile
vita:
	$(MAKE) -f misc/vita/Makefile
ps1:
	cmake --preset default misc/ps1
	cmake --build misc/ps1/build
ps2:
	$(MAKE) -f misc/ps2/Makefile
ps3:
	$(MAKE) -f misc/ps3/Makefile
xbox:
	$(MAKE) -f misc/xbox/Makefile
xbox360:
	$(MAKE) -f misc/xbox360/Makefile
n64:
	$(MAKE) -f misc/n64/Makefile
ds:
	$(MAKE) -f misc/ds/Makefile
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
macclassic:
	$(MAKE) -f misc/macclassic/Makefile
	
clean:
	$(DEL) $(OBJECTS)


$(ENAME): $(BUILD_DIR) $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@$(OEXT) $(OBJECTS) $(LIBS)
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)


# === Compiling with dependency tracking ===
# NOTE: Tracking dependencies might not work on older systems - disable this if so
ifeq ($(TRACK_DEPENDENCIES), 1)
DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d
DEPFILES := $(patsubst $(SOURCE_DIR)/%.c, $(BUILD_DIR)/%.d, $(C_SOURCES))
$(DEPFILES):

$(C_OBJECTS): $(BUILD_DIR)/%.o : $(SOURCE_DIR)/%.c $(BUILD_DIR)/%.d
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

include $(wildcard $(DEPFILES))
# === Compiling WITHOUT dependency tracking ===
else
$(C_OBJECTS): $(BUILD_DIR)/%.o : $(SOURCE_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@
endif
	
# === Platform specific file compiling ===
$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.m
	$(CC) $(CFLAGS) -c $< -o $@
	
$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.cpp
	$(CC) $(CFLAGS) -c $< -o $@
