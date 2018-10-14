CC=gcc
LIBS=-lGL -lcurl -lopenal -lpthread -lX11 -lm
CFLAGS=-g -w
SOURCES=$(wildcard *.c)
OBJECTS=$(patsubst %.c, %.o, $(SOURCES))

EXECUTABLE=ClassiCube

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LIBS)

$(OBJECTS): %.o : %.c
	$(CC) $(CFLAGS) -c $< $(LIBS) -o $@

clean:
	rm -rf $(EXECUTABLE) $(OBJECTS)
