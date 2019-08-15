SOURCES=$(wildcard *.c)
OBJECTS=$(patsubst %.c, %.o, $(SOURCES))
COMMITSHA=$(shell git rev-parse --short HEAD)
ENAME=ClassiCube
DEL=rm
JOBS=1
CC=gcc
CFLAGS=-w -g -pipe -rdynamic
LDFLAGS=-g -rdynamic

ifndef $(PLAT)
	ifeq ($(OS),Windows_NT)
		PLAT=mingw
	else
		PLAT=$(shell uname -s | tr '[:upper:]' '[:lower:]')
	endif
endif

ifeq ($(PLAT),web)
CC=emcc
OEXT=.html
CFLAGS=-w -g
LDFLAGS=-s WASM=1 -s NO_EXIT_RUNTIME=1 -s FETCH=1 --preload-file texpacks/default.zip@texpacks/default.zip
endif

ifeq ($(PLAT),mingw)
OEXT=.exe
CFLAGS=-w -g -pipe -DUNICODE
LDFLAGS=-g
LIBS=-mwindows -lws2_32 -lwininet -lwinmm -limagehlp -lcrypt32 -ld3d9
endif

ifeq ($(PLAT),linux)
LIBS=-lX11 -lpthread -lGL -lm -lopenal -ldl -lcurl
endif

ifeq ($(PLAT),sunos)
LIBS=-lm -lsocket -lX11 -lGL -lcurl -lopenal
endif

ifeq ($(PLAT),darwin)
LIBS=-lcurl
LDFLAGS=-rdynamic -framework Carbon -framework AGL -framework OpenAL -framework OpenGL
endif

ifeq ($(PLAT),freebsd)
CC=clang
CFLAGS=-w -g -pipe -rdynamic -I /usr/local/include
LDFLAGS=-L /usr/local/lib
LIBS=-lcurl -lexecinfo -lopenal -lGL -lX11 -lm -lpthread
endif

ifeq ($(PLAT),openbsd)
CFLAGS=-w -g -pipe -rdynamic -I /usr/X11R6/include -I /usr/local/include
LDFLAGS=-L /usr/X11R6/lib -L /usr/local/lib
LIBS=-lcurl -lexecinfo -lopenal -lGL -lX11
endif

ifeq ($(PLAT),netbsd)
CFLAGS=-w -g -pipe -rdynamic -I /usr/X11R7/include -I /usr/pkg/include
LDFLAGS=-L /usr/X11R7/lib -L /usr/pkg/lib
LIBS=-lcurl -lexecinfo -lopenal -lGL -lX11
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
darwin:
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
