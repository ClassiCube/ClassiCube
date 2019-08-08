
ifeq ($(SYSTEM_FREETYPE),)
	SOURCES := $(wildcard *.c)
	EXTRA_FLAGS := -DCC_BUILD_CUSTOM_FREETYPE
	EXTRA_LIBS :=
else
	SOURCES := $(wildcard [A-Z]*.c)
	EXTRA_FLAGS :=
	EXTRA_LIBS := -lfreetype
endif
OBJECTS := $(patsubst %.c, %.o, $(SOURCES))
COMMITSHA := $(shell git rev-parse --short HEAD)
ENAME=ClassiCube
DEL=$(RM)

JOBS=1
CC=gcc
CFLAGS := -g -pipe -rdynamic $(EXTRA_FLAGS)
LDFLAGS := -g -rdynamic

ifeq ($(PLAT),)
	ifeq ($(OS),Windows_NT)
		PLAT=mingw
	else
		PLAT=$(shell uname -s | tr '[:upper:]' '[:lower:]')
	endif
endif

ifeq ($(PLAT),web)
CC=emcc
OEXT=.html
CFLAGS=-w -g $(EXTRA_FLAGS)
LDFLAGS=-s WASM=1 -s NO_EXIT_RUNTIME=1 -s FETCH=1 --preload-file texpacks/default.zip@texpacks/default.zip $(EXTRA_LIBS)
endif

ifeq ($(PLAT),mingw)
OEXT=.exe
CFLAGS=-w -g -pipe -DUNICODE $(EXTRA_FLAGS)
LDFLAGS=-g
LIBS=-mwindows -lws2_32 -lwininet -lwinmm -limagehlp -lcrypt32 -ld3d9 $(EXTRA_LIBS)
endif

ifeq ($(PLAT),linux)
LIBS=-lX11 -lpthread -lGL -lm -lopenal -ldl -lcurl $(EXTRA_LIBS)
endif

ifeq ($(PLAT),sunos)
LIBS=-lm -lsocket -lX11 -lGL -lcurl -lopenal $(EXTRA_LIBS)
endif

ifeq ($(PLAT),darwin)
CC := clang
LIBS=-lcurl
ifeq ($(SYSTEM_FREETYPE),)

CFLAGS := -g -O2 $(shell pkg-config --cflags sdl2) -D_XOPEN_SOURCE $(EXTRA_FLAGS) -DGL_SILENCE_DEPRECATION -std=c99 -Wall -Wno-missing-braces
LDFLAGS:= -framework OpenAL -framework OpenGL -framework  ApplicationServices $(shell pkg-config --libs sdl2)
else
CFLAGS := -g -O2 $(shell pkg-config --cflags sdl2 freetype2) -D_XOPEN_SOURCE -DGL_SILENCE_DEPRECATION -std=c99 -Wall -Wno-missing-braces
LDFLAGS:= -framework OpenAL -framework OpenGL -framework  ApplicationServices $(shell pkg-config --libs sdl2 freetype2)
endif

endif

ifeq ($(PLAT),freebsd)
CC=clang
CFLAGS=-w -g -pipe -rdynamic -I /usr/local/include $(EXTRA_FLAGS)
LDFLAGS=-L /usr/local/lib
LIBS=-lcurl -lexecinfo -lopenal -lGL -lX11 -lm -lpthread $(EXTRA_LIBS)
endif

ifeq ($(PLAT),openbsd)
CFLAGS=-w -g -pipe -rdynamic -I /usr/X11R6/include -I /usr/local/include $(EXTRA_FLAGS)
LDFLAGS=-L /usr/X11R6/lib -L /usr/local/lib
LIBS=-lcurl -lexecinfo -lopenal -lGL -lX11 $(EXTRA_LIBS)
endif

ifeq ($(PLAT),netbsd)
CFLAGS=-w -g -pipe -rdynamic -I /usr/X11R7/include -I /usr/pkg/include $(EXTRA_FLAGS)
LDFLAGS=-L /usr/X11R7/lib -L /usr/pkg/lib
LIBS=-lcurl -lexecinfo -lopenal -lGL -lX11 $(EXTRA_LIBS)
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
solaris:
	$(MAKE) $(ENAME) PLAT=sunos -j$(JOBS)
darwin:
	$(MAKE) $(ENAME) PLAT=darwin -j$(JOBS)
osx:
	$(MAKE) $(ENAME) PLAT=darwin -j$(JOBS)
freebsd:
	$(MAKE) $(ENAME) PLAT=freebsd -j$(JOBS)
openbsd:
	$(MAKE) $(ENAME) PLAT=openbsd -j$(JOBS)
netbsd:
	$(MAKE) $(ENAME) PLAT=netbsd -j$(JOBS)
	
clean:
	$(DEL) $(OBJECTS)

$(ENAME): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@$(OEXT) $(OBJECTS) $(LIBS)

$(OBJECTS): %.o : %.c
	$(CC) $(CFLAGS) -DCC_COMMIT_SHA=\"$(COMMITSHA)\" -c $< -o $@
