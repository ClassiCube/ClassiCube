ifeq ($(strip $(PS2SDK)),)
$(error "PS2SDK must be set in your environment")
endif
BUILD_DIR = build-ps2

CFILES 	:= $(wildcard src/*.c)
OBJS	:=$(patsubst %.c, %.o, $(CFILES))

EE_BIN = ClassiCube-ps2.elf
EE_OBJS = $(OBJS) DEV9_irx.o NETMAN_irx.o SMAP_irx.o USBD_irx.o BDM_irx.o BDMFS_FATFS_irx.o USBMASS_BD_irx.o USBHDFSD_irx.o
EE_LIBS = -lpatches -lpad -lpacket -ldma -lgraph -ldraw -lc -lps2ip -lnetman -lmc
EE_CFLAGS = -DPLAT_PS2

all: $(EE_BIN)

clean:
	rm -f $(EE_BIN) $(EE_OBJS)
	
# Networking IRX modules
DEV9_irx.c: $(PS2SDK)/iop/irx/ps2dev9.irx
	bin2c $< DEV9_irx.c DEV9_irx

NETMAN_irx.c: $(PS2SDK)/iop/irx/netman.irx
	bin2c $< NETMAN_irx.c NETMAN_irx

SMAP_irx.c: $(PS2SDK)/iop/irx/smap.irx
	bin2c $< SMAP_irx.c SMAP_irx
	
# USB storage IRX modules
USBD_irx.c: $(PS2SDK)/iop/irx/usbd.irx
	bin2c $< USBD_irx.c USBD_irx

BDM_irx.c: $(PS2SDK)/iop/irx/bdm.irx
	bin2c $< BDM_irx.c BDM_irx

BDMFS_FATFS_irx.c: $(PS2SDK)/iop/irx/bdmfs_fatfs.irx
	bin2c $< BDMFS_FATFS_irx.c BDMFS_FATFS_irx

USBMASS_BD_irx.c: $(PS2SDK)/iop/irx/usbmass_bd.irx
	bin2c $< USBMASS_BD_irx.c USBMASS_BD_irx

USBHDFSD_irx.c: $(PS2SDK)/iop/irx/usbhdfsd.irx
	bin2c $< USBHDFSD_irx.c USBHDFSD_irx


include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
