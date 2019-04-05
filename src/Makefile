SOURCES=$(wildcard *.c)
OBJECTS=$(patsubst %.c, %.o, $(SOURCES))
COMMITSHA=$(shell git rev-parse --short HEAD)
ENAME=ClassiCube
PLAT=ns
DEL=rm
JOBS=1

ifeq ($(OS),Windows_NT)
DEL=del
endif

CC_web=emcc
LIBS_web=
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

default: $(PLAT)

web:
	$(MAKE) $(ENAME) PLAT=web -j$(JOBS)

linux:
	$(MAKE) $(ENAME) PLAT=linux -j$(JOBS)

mingw:
	$(MAKE) $(ENAME) PLAT=mingw -j$(JOBS)

clean:
	$(DEL) $(OBJECTS)

ns:
	$(error Target platform not selected)

CC=$(CC_$(PLAT))
LIBS=$(LIBS_$(PLAT))
OEXT=$(OEXT_$(PLAT))
CFLAGS=$(CFLAGS_$(PLAT))
LDFLAGS=$(LDFLAGS_$(PLAT))

$(ENAME): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@$(OEXT) $(OBJECTS) $(LIBS)

$(OBJECTS): %.o : %.c
	$(CC) $(CFLAGS) -DCC_COMMIT_SHA=\"$(COMMITSHA)\" -c $< $(LIBS) -o $@
