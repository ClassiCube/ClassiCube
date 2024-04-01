SOURCE_DIR  := src
BUILD_DIR   := build
C_SOURCES   := $(wildcard $(SOURCE_DIR)/*.c)
C_OBJECTS   := $(patsubst $(SOURCE_DIR)/%.c, $(BUILD_DIR)/%.o, $(C_SOURCES))

OBJECTS := $(C_OBJECTS)
ENAME   = ClassiCube
DEL     = rm -f
CFLAGS  = -g -pipe -fno-math-errno
LDFLAGS = -g -rdynamic

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
LIBS    = -lX11 -lXi -lpthread -lGL -ldl
endif

ifeq ($(PLAT),sunos)
CFLAGS  = -g -pipe -fno-math-errno
LIBS    = -lsocket -lX11 -lXi -lGL
endif

ifeq ($(PLAT),darwin)
OBJECTS += $(BUILD_DIR)/interop_cocoa.o
CFLAGS  = -g -pipe -fno-math-errno
LIBS    =
LDFLAGS = -rdynamic -framework Cocoa -framework OpenGL -framework IOKit -lobjc
endif

ifeq ($(PLAT),freebsd)
CFLAGS  = -g -pipe -I /usr/local/include -fno-math-errno
LDFLAGS = -L /usr/local/lib -rdynamic
LIBS    = -lexecinfo -lGL -lX11 -lXi -lpthread
endif

ifeq ($(PLAT),openbsd)
CFLAGS  = -g -pipe -I /usr/X11R6/include -I /usr/local/include -fno-math-errno
LDFLAGS = -L /usr/X11R6/lib -L /usr/local/lib -rdynamic
LIBS    = -lexecinfo -lGL -lX11 -lXi -lpthread
endif

ifeq ($(PLAT),netbsd)
CFLAGS  = -g -pipe -I /usr/X11R7/include -I /usr/pkg/include -fno-math-errno
LDFLAGS = -L /usr/X11R7/lib -L /usr/pkg/lib -rdynamic
LIBS    = -lexecinfo -lGL -lX11 -lXi -lpthread
endif

ifeq ($(PLAT),dragonfly)
CFLAGS  = -g -pipe -I /usr/local/include -fno-math-errno
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

# consoles builds require special handling, so are moved to
#  separate makefiles to avoid having one giant messy makefile
dreamcast:
	$(MAKE) -f misc/dreamcast/Makefile PLAT=dreamcast
psp:
	$(MAKE) -f misc/psp/Makefile PLAT=psp
vita:
	$(MAKE) -f misc/vita/Makefile PLAT=vita
ps3:
	$(MAKE) -f misc/ps3/Makefile PLAT=ps3
ps1:
	cmake --preset default misc/ps1
	cmake --build misc/ps1/build
ps2:
	$(MAKE) -f misc/ps2/Makefile PLAT=ps2
xbox:
	$(MAKE) -f misc/xbox/Makefile PLAT=xbox
xbox360:
	$(MAKE) -f misc/xbox360/Makefile PLAT=xbox360
n64:
	$(MAKE) -f misc/n64/Makefile PLAT=n64
3ds:
	$(MAKE) -f misc/3ds/Makefile PLAT=3ds
wii:
	$(MAKE) -f misc/wii/Makefile PLAT=wii
gamecube:
	$(MAKE) -f misc/gc/Makefile PLAT=gamecube
wiiu:
	$(MAKE) -f misc/wiiu/Makefile PLAT=wiiu
switch:
	$(MAKE) -f misc/switch/Makefile PLAT=switch
ds:
	$(MAKE) -f misc/ds/Makefile PLAT=ds
os/2:
	$(MAKE) -f misc/os2/Makefile PLAT=os2
	
clean:
	$(DEL) $(OBJECTS)

$(ENAME): $(BUILD_DIR) $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@$(OEXT) $(OBJECTS) $(LIBS)
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(C_OBJECTS): $(BUILD_DIR)/%.o : $(SOURCE_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@
	
$(BUILD_DIR)/interop_cocoa.o: $(SOURCE_DIR)/interop_cocoa.m
	$(CC) $(CFLAGS) -c $< -o $@
	
$(BUILD_DIR)/interop_BeOS.o: $(SOURCE_DIR)/interop_BeOS.cpp
	$(CC) $(CFLAGS) -c $< -o $@
