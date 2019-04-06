SOURCES=$(wildcard *.c)
OBJECTS=$(patsubst %.c, %.o, $(SOURCES))
COMMITSHA=$(shell git rev-parse --short HEAD)
ENAME=ClassiCube
DEL=rm
JOBS=1

ifndef $(PLAT)
	ifeq ($(OS),Windows_NT)
		PLAT=mingw
	else
		PLAT=$(shell uname -s | tr -s '[:upper:]' '[:lower:]')
	endif
endif

CC_web=emcc
OEXT_web=.html
CFLAGS_web=-w -g
LDFLAGS_web=-s WASM=1 -s NO_EXIT_RUNTIME=1 -s FETCH=1 --preload-file texpacks/default.zip@texpacks/default.zip

CC_mingw=gcc
LIBS_mingw=-mwindows -lws2_32 -lwininet -lwinmm -limagehlp -lcrypt32 -ld3d9
OEXT_mingw=.exe
CFLAGS_mingw=-w -g -pipe

CC_linux=gcc
LIBS_linux=-lX11 -lpthread -lGL -lm -lopenal -ldl -lcurl
CFLAGS_linux=-w -g -pipe

CC_sunos=gcc
LIBS_sunos=-lm -lsocket -lX11 -lGL -lcurl -lopenal
CFLAGS_sunos=-w -g -pipe

CC_darwin=gcc
LIBS_darwin=-lcurl
LDFLAGS_darwin=-framework Carbon -framework AGL -framework OpenAL -framework OpenGL
CFLAGS_darwin=-w -g -pipe

BSDLIBS=-lcurl -lexecinfo -lopenal -lGL -lX11

CC_freebsd=gcc
LIBS_freebsd=$(BSDLIBS) -lm -lpthread
INCDIRS_freebsd=-I /usr/local/include -L /usr/Xlocal/lib
CFLAGS_freebsd=-w -g -pipe

CC_openbsd=gcc
LIBS_openbsd=$(BSDLIBS)
INCDIRS_openbsd=-isystem /usr/X11R6/include -isystem /usr/local/include
LDFLAGS_openbsd=-L /usr/X11R6/lib -L /usr/local/lib

CC_netbsd=gcc
LIBS_netbsd=$(BSDLIBS)
INCDIRS_netbsd=-I /usr/X11R7/include -I /usr/pkg/include
LDFLAGS_netbsd=-L /usr/X11R7/lib -L /usr/pkg/lib

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

CC=$(CC_$(PLAT))
LIBS=$(LIBS_$(PLAT))
OEXT=$(OEXT_$(PLAT))
CFLAGS=$(CFLAGS_$(PLAT))
LDFLAGS=$(LDFLAGS_$(PLAT))
INCDIRS=$(INCDIRS_$(PLAT))

$(ENAME): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@$(OEXT) $(OBJECTS) $(LIBS)

$(OBJECTS): %.o : %.c
	$(CC) $(CFLAGS) $(INCDIRS) -DCC_COMMIT_SHA=\"$(COMMITSHA)\" -c $< -o $@
