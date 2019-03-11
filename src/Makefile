CC=gcc
LIBS=-lX11 -lpthread -lGL -lm -lcurl -lopenal -ldl
CFLAGS=-g -w -no-pie -rdynamic -pipe
SOURCES=$(wildcard *.c)
OBJECTS=$(patsubst %.c, %.o, $(SOURCES))
COMMITSHA=$(shell git rev-parse --short HEAD)

EXECUTABLE=ClassiCube

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LIBS)

$(OBJECTS): %.o : %.c
	$(CC) $(CFLAGS) -DCC_COMMIT_SHA=\"$(COMMITSHA)\" -c $< $(LIBS) -o $@

clean:
	rm -f $(EXECUTABLE) $(OBJECTS)
