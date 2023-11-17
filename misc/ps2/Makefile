BUILD_DIR = build-ps2

CFILES 	:= $(wildcard src/*.c)
OBJS	:=$(patsubst %.c, %.o, $(CFILES))

EE_BIN = ClassiCube-ps2.elf
EE_OBJS = $(OBJS)
EE_LIBS = -ldebug -lpad -lc $(PS2DEV)/gsKit/lib/libgskit.a
EE_CFLAGS = -DPLAT_PS2
EE_INCS = -I$(PS2DEV)/gsKit/include

all: $(EE_BIN)

clean:
	rm -f $(EE_BIN) $(EE_OBJS)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
