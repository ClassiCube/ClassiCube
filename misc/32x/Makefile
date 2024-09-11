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

TARGET = ClassiCube
LIBS = $(LIBPATH) -lc -lgcc -lgcc-Os-4-200 -lnosys

SOURCE_DIRS = ../../src
C_FILES = $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
OBJS = sh2_crt0.o main_32x.o hw_32x.o $(notdir $(C_FILES:%.c=%.o))

all: m68k_crt0.bin m68k_crt1.bin $(TARGET).bin

$(TARGET).bin: $(TARGET).elf
	$(SHOBJC) -O binary $< temp.bin
	$(DD) if=temp.bin of=$@ bs=64K conv=sync

$(TARGET).elf: $(OBJS)
	$(SHCC) $(SHLDFLAGS) $(OBJS) $(LIBS) -o $(TARGET).elf

m68k_crt0.bin: m68k_crt0.s
	$(MDAS) $(MDASFLAGS) -o m68k_crt0.o m68k_crt0.s
	$(MDLD) $(MDLDFLAGS) -o m68k_crt0.bin m68k_crt0.o

m68k_crt1.bin: m68k_crt1.s
	$(MDAS) $(MDASFLAGS) -o m68k_crt1.o m68k_crt1.s
	$(MDLD) $(MDLDFLAGS) -o m68k_crt1.bin m68k_crt1.o

hw_32x.o: hw_32x.c
	$(SHCC) $(SHHWFLAGS) $(INCPATH) $< -o $@

%.o: %.c
	$(SHCC) $(SHCCFLAGS) $(INCPATH) $< -o $@

%.o: ../../src/%.c
	$(SHCC) $(SHCCFLAGS) $(INCPATH) $< -o $@

%.o: %.s
	$(SHAS) $(SHASFLAGS) $(INCPATH) $< -o $@

clean:
	$(RM) *.o *.bin *.elf output.map
