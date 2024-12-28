ifdef $(GENDEV)
ROOTDIR = $(GENDEV)
else
ROOTDIR = /opt/toolchains/sega
endif

LDSCRIPTSDIR = $(ROOTDIR)/ldscripts

LIBPATH = -L$(ROOTDIR)/sh-elf/lib -L$(ROOTDIR)/sh-elf/lib/gcc/sh-elf/4.6.2 -L$(ROOTDIR)/sh-elf/sh-elf/lib
INCPATH = -I. -I../include -I$(ROOTDIR)/sh-elf/include -I$(ROOTDIR)/sh-elf/sh-elf/include

SHCCFLAGS = -m2 -mb -Ofast -Wall -g -c -fomit-frame-pointer -DPLAT_32X -ffunction-sections -fdata-sections
SHHWFLAGS = -m2 -mb -O1 -Wall -g -c -fomit-frame-pointer
SHLDFLAGS = -T $(LDSCRIPTSDIR)/mars.ld -Wl,-Map=output.map -nostdlib -Wl,--gc-sections
SHASFLAGS = --big

MDLDFLAGS = -T $(LDSCRIPTSDIR)/md.ld --oformat binary
MDASFLAGS = -m68000 --register-prefix-optional

SHPREFIX = $(ROOTDIR)/sh-elf/bin/sh-elf-
SHCC = $(SHPREFIX)gcc
SHAS = $(SHPREFIX)as
SHLD = $(SHPREFIX)ld
SHOBJC = $(SHPREFIX)objcopy

MDPREFIX = $(ROOTDIR)/m68k-elf/bin/m68k-elf-
MDAS = $(MDPREFIX)as
MDLD = $(MDPREFIX)ld

DD = dd
RM = rm -f

TARGET = ClassiCube-32x
LIBS = $(LIBPATH) -lc -lgcc -lgcc-Os-4-200 -lnosys
BUILD_DIR = build-32x

SOURCE_DIRS = src misc/32x
C_FILES = $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
S_FILES = misc/32x/sh2_crt0.s
OBJS = $(addprefix $(BUILD_DIR)/, $(notdir $(C_FILES:%.c=%.o) $(S_FILES:%.s=%.o)))


#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all: $(BUILD_DIR) m68k_crt0.bin m68k_crt1.bin $(TARGET).bin

clean:
	$(RM) *.o *.bin *.elf output.map

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)


#---------------------------------------------------------------------------------
# binary generation
#---------------------------------------------------------------------------------
$(TARGET).bin: $(TARGET).elf
	$(SHOBJC) -O binary $< temp.bin
	$(DD) if=temp.bin of=$@ bs=64K conv=sync

$(TARGET).elf: $(OBJS)
	$(SHCC) $(SHLDFLAGS) $(OBJS) $(LIBS) -o $(TARGET).elf


$(BUILD_DIR)/m68k_crt0.o: misc/32x/m68k_crt0.s
	$(MDAS) $(MDASFLAGS) $< -o $@

m68k_crt0.bin: $(BUILD_DIR)/m68k_crt0.o
	$(MDLD) $(MDLDFLAGS) $< -o $@

$(BUILD_DIR)/m68k_crt1.o: misc/32x/m68k_crt1.s
	$(MDAS) $(MDASFLAGS) $< -o $@

m68k_crt1.bin: $(BUILD_DIR)/m68k_crt1.o
	$(MDLD) $(MDLDFLAGS) $< -o $@


#---------------------------------------------------------------------------------
# object generation
#---------------------------------------------------------------------------------
$(BUILD_DIR)/%.o: src/%.c
	$(SHCC) $(SHCCFLAGS) $(INCPATH) $< -o $@

$(BUILD_DIR)/%.o: misc/32x/%.c
	$(SHCC) $(SHCCFLAGS) $(INCPATH) $< -o $@

$(BUILD_DIR)/%.o: misc/32x/%.s
	$(SHAS) $(SHASFLAGS) $(INCPATH) $< -o $@
