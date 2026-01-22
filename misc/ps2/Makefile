ifeq ($(strip $(PS2SDK)),)
$(error "PS2SDK must be set in your environment")
endif

.SUFFIXES:


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
# Name of the final output
TARGET 	= ClassiCube-ps2
# List of directories containing source code
SOURCE_DIRS	:= src src/ps2 third_party/bearssl
# Directory where object files are placed
BUILD_DIR = build/ps2


#---------------------------------------------------------------------------------
# Compilable files
#---------------------------------------------------------------------------------
S_FILES := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.S))
C_FILES := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
OBJS 	:= $(addprefix $(BUILD_DIR)/, $(notdir $(C_FILES:%.c=%.o) $(S_FILES:%.S=%.o)))

# Dependency tracking
DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d
DEPFILES := $(OBJS:%.o=%.d)


#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------
IOP_MODS := DEV9_irx.o NETMAN_irx.o SMAP_irx.o USBD_irx.o BDM_irx.o BDMFS_FATFS_irx.o USBMASS_BD_irx.o USBHDFSD_irx.o USBMOUSE_irx.o USBKBD_irx.o

EE_OBJS = $(OBJS) $(patsubst %.o, $(BUILD_DIR)/%.o, $(IOP_MODS))
EE_LIBS = -lpatches -lpad -ldma -lgraph -ldraw -lc -lps2ip -lnetman -lmc -lmouse -lkbd

EE_INCS 	:= -I$(PS2SDK)/ee/include -I$(PS2SDK)/common/include
EE_CFLAGS 	:= -D_EE -G0 -O2 -Wall -gdwarf-2 -gz -DPLAT_PS2
EE_LDFLAGS 	:= -L$(PS2SDK)/ee/lib -Wl,-zmax-page-size=128
EE_LINKFILE := $(PS2SDK)/ee/startup/linkfile


#---------------------------------------------------------------------------------
# Compiler tools
#---------------------------------------------------------------------------------
EE_CC 	 = mips64r5900el-ps2-elf-gcc
EE_STRIP = mips64r5900el-ps2-elf-strip


#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all: $(BUILD_DIR) $(TARGET)-min.elf

clean:
	rm -f $(TARGET)-min.elf $(TARGET).elf $(EE_OBJS)
	
$(BUILD_DIR):
	mkdir -p $@


#---------------------------------------------------------------------------------
# IRX modules
#---------------------------------------------------------------------------------
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


#---------------------------------------------------------------------------------
# executable generation
#---------------------------------------------------------------------------------
$(TARGET).elf : $(EE_OBJS)
	$(EE_CC) -T$(EE_LINKFILE) -O2 -o $(TARGET).elf $(EE_OBJS) $(EE_LDFLAGS) $(EE_LIBS)
	
$(TARGET)-min.elf : $(TARGET).elf
	$(EE_STRIP) $(TARGET).elf -o $(TARGET)-min.elf


#---------------------------------------------------------------------------------
# object generation
#---------------------------------------------------------------------------------
$(BUILD_DIR)/%.o: src/%.c
	$(EE_CC) $(DEPFLAGS) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

$(BUILD_DIR)/%.o: src/ps2/%.c
	$(EE_CC) $(DEPFLAGS) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

$(BUILD_DIR)/%.o: third_party/bearssl/%.c
	$(EE_CC) $(DEPFLAGS) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

$(BUILD_DIR)/%.o: src/ps2/%.S
	$(EE_CC) $(DEPFLAGS) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

$(BUILD_DIR)/%.o: $(BUILD_DIR)/%.c # IOP modules
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@


#---------------------------------------------------------------------------------
# Dependency tracking
#---------------------------------------------------------------------------------
$(DEPFILES):

include $(wildcard $(DEPFILES))

