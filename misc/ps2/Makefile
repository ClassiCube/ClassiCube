ifeq ($(strip $(PS2SDK)),)
$(error "PS2SDK must be set in your environment")
endif
BUILD_DIR = build-ps2

CFILES 	:= $(wildcard src/*.c)
OBJS	:=$(patsubst %.c, %.o, $(CFILES))
IOP_MODS:= DEV9_irx.o NETMAN_irx.o SMAP_irx.o USBD_irx.o BDM_irx.o BDMFS_FATFS_irx.o USBMASS_BD_irx.o USBHDFSD_irx.o USBMOUSE_irx.o USBKBD_irx.o

EE_BIN = ClassiCube-ps2.elf
EE_OBJS = $(OBJS) $(patsubst %.o, $(BUILD_DIR)/%.o, $(IOP_MODS))
EE_LIBS = -lpatches -lpad -lpacket -ldma -lgraph -ldraw -lc -lps2ip -lnetman -lmc -lmouse -lkbd
EE_CFLAGS = -DPLAT_PS2

all: $(BUILD_DIR) $(EE_BIN)

clean:
	rm -f $(EE_BIN) $(EE_OBJS)
	
$(BUILD_DIR):
	mkdir -p $@
	
# Networking IRX modules
$(BUILD_DIR)/DEV9_irx.c: $(PS2SDK)/iop/irx/ps2dev9.irx
	bin2c $< $@ DEV9_irx

$(BUILD_DIR)/NETMAN_irx.c: $(PS2SDK)/iop/irx/netman.irx
	bin2c $< $@ NETMAN_irx

$(BUILD_DIR)/SMAP_irx.c: $(PS2SDK)/iop/irx/smap.irx
	bin2c $< $@ SMAP_irx
	
# USB storage IRX modules
$(BUILD_DIR)/USBD_irx.c: $(PS2SDK)/iop/irx/usbd.irx
	bin2c $< $@ USBD_irx

$(BUILD_DIR)/BDM_irx.c: $(PS2SDK)/iop/irx/bdm.irx
	bin2c $< $@ BDM_irx

$(BUILD_DIR)/BDMFS_FATFS_irx.c: $(PS2SDK)/iop/irx/bdmfs_fatfs.irx
	bin2c $< $@ BDMFS_FATFS_irx

$(BUILD_DIR)/USBMASS_BD_irx.c: $(PS2SDK)/iop/irx/usbmass_bd.irx
	bin2c $< $@ USBMASS_BD_irx

$(BUILD_DIR)/USBHDFSD_irx.c: $(PS2SDK)/iop/irx/usbhdfsd.irx
	bin2c $< $@ USBHDFSD_irx

# USB input IRX modules
$(BUILD_DIR)/USBMOUSE_irx.c: $(PS2SDK)/iop/irx/ps2mouse.irx
	bin2c $< $@ USBMOUSE_irx

$(BUILD_DIR)/USBKBD_irx.c: $(PS2SDK)/iop/irx/ps2kbd.irx
	bin2c $< $@ USBKBD_irx


include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
