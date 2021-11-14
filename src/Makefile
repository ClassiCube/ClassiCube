SOURCES=$(wildcard *.c)
OBJECTS=$(patsubst %.c, %.o, $(SOURCES))
ENAME=ClassiCube
DEL=rm
JOBS=1
CC=cc
CFLAGS=-g -pipe -rdynamic -fno-math-errno
LDFLAGS=-g -rdynamic

ifndef $(PLAT)
	ifeq ($(OS),Windows_NT)
		PLAT=mingw
	else
		PLAT=$(shell uname -s | tr '[:upper:]' '[:lower:]')
	endif
	
	ifeq ($(PLAT),darwin)
		ifeq ($(shell uname -m), x86_64)
			PLAT=mac_x64
		else
			PLAT=mac_x32
		endif
	endif
endif

ifeq ($(PLAT),web)
CC=emcc
OEXT=.html
CFLAGS=-g
LDFLAGS=-s WASM=1 -s NO_EXIT_RUNTIME=1 --preload-file texpacks/default.zip@texpacks/default.zip
endif

ifeq ($(PLAT),mingw)
CC=gcc
OEXT=.exe
CFLAGS=-g -pipe -DUNICODE -fno-math-errno
LDFLAGS=-g
LIBS=-mwindows -lws2_32 -lwininet -lwinmm -limagehlp -lcrypt32 -ld3d9
endif

ifeq ($(PLAT),linux)
LIBS=-lX11 -lXi -lpthread -lGL -lm -ldl
endif

ifeq ($(PLAT),sunos)
CC=gcc
CFLAGS=-g -pipe -fno-math-errno
LIBS=-lm -lsocket -lX11 -lXi -lGL
endif

ifeq ($(PLAT),mac_x32)
LIBS=
CFLAGS=-g -m32 -pipe -fno-math-errno
LDFLAGS=-rdynamic -framework Carbon -framework AGL -framework OpenGL -framework IOKit
endif

ifeq ($(PLAT),mac_x64)
SOURCES+=interop_cocoa.m
CFLAGS=-g -m64 -pipe -fno-math-errno
LIBS=
LDFLAGS=-rdynamic -framework Cocoa -framework OpenGL -framework IOKit -lobjc
endif

ifeq ($(PLAT),freebsd)
CFLAGS=-g -pipe -I /usr/local/include -fno-math-errno
LDFLAGS=-L /usr/local/lib -rdynamic
LIBS=-lexecinfo -lGL -lX11 -lXi -lm -lpthread
endif

ifeq ($(PLAT),openbsd)
CFLAGS=-g -pipe -I /usr/X11R6/include -I /usr/local/include -fno-math-errno
LDFLAGS=-L /usr/X11R6/lib -L /usr/local/lib -rdynamic
LIBS=-lexecinfo -lGL -lX11 -lXi -lm -lpthread
endif

ifeq ($(PLAT),netbsd)
CFLAGS=-g -pipe -I /usr/X11R7/include -I /usr/pkg/include -fno-math-errno
LDFLAGS=-L /usr/X11R7/lib -L /usr/pkg/lib -rdynamic
LIBS=-lexecinfo -lGL -lX11 -lXi -lpthread
endif

ifeq ($(PLAT),dragonfly)
CFLAGS=-g -pipe -I /usr/local/include -fno-math-errno
LDFLAGS=-L /usr/local/lib -rdynamic
LIBS=-lexecinfo -lGL -lX11 -lXi -lm -lpthread
endif

ifeq ($(PLAT),haiku)
CFLAGS=-g -pipe -fno-math-errno
LDFLAGS=-g
LIBS=-lm -lexecinfo -lGL -lnetwork -lSDL2
endif

ifeq ($(OS),Windows_NT)
DEL=del
endif

default: $(PLAT)

web:
	$(MAKE) $(ENAME) PLAT=web -j$(JOBS)
linux:
	$(MAKE) $(ENAME) PLAT=linux -j$(JOBS)
mingw:
	$(MAKE) $(ENAME) PLAT=mingw -j$(JOBS)
sunos:
	$(MAKE) $(ENAME) PLAT=sunos -j$(JOBS)
mac_x32:
	$(MAKE) $(ENAME) PLAT=mac_x32 -j$(JOBS)
mac_x64:
	$(MAKE) $(ENAME) PLAT=mac_x64 -j$(JOBS)
freebsd:
	$(MAKE) $(ENAME) PLAT=freebsd -j$(JOBS)
openbsd:
	$(MAKE) $(ENAME) PLAT=openbsd -j$(JOBS)
netbsd:
	$(MAKE) $(ENAME) PLAT=netbsd -j$(JOBS)
dragonfly:
	$(MAKE) $(ENAME) PLAT=dragonfly -j$(JOBS)
haiku:
	$(MAKE) $(ENAME) PLAT=haiku -j$(JOBS)
	
clean:
	$(DEL) $(OBJECTS)

$(ENAME): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@$(OEXT) $(OBJECTS) $(LIBS)

$(OBJECTS): %.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@
