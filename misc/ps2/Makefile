BUILD_DIR = build-ps2

CFILES 	:= $(wildcard src/*.c)
OBJS	:=$(patsubst %.c, %.o, $(CFILES))

EE_BIN = ClassiCube-ps2.elf
EE_OBJS = $(OBJS) DEV9_irx.o NETMAN_irx.o SMAP_irx.o
EE_LIBS = -lpatches -lpad -lpacket -ldma -lgraph -ldraw -lc
EE_CFLAGS = -DPLAT_PS2

all: $(EE_BIN)

clean:
	rm -f $(EE_BIN) $(EE_OBJS)
	
DEV9_irx.c: $(PS2SDK)/iop/irx/ps2dev9.irx
	bin2c $< DEV9_irx.c DEV9_irx

NETMAN_irx.c: $(PS2SDK)/iop/irx/netman.irx
	bin2c $< NETMAN_irx.c NETMAN_irx

SMAP_irx.c: $(PS2SDK)/iop/irx/smap.irx
	bin2c $< SMAP_irx.c SMAP_irx

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
