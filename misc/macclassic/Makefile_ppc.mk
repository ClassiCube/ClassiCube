# path to RETRO68
RETRO68=../Retro68-build/toolchain

PREFIX=$(RETRO68)/powerpc-apple-macos
CC=$(RETRO68)/bin/powerpc-apple-macos-gcc
CXX=$(RETRO68)/bin/powerpc-apple-macos-g++
MakePEF=$(RETRO68)/bin/MakePEF

REZ=$(RETRO68)/bin/Rez
RINCLUDES=$(PREFIX)/RIncludes
REZFLAGS=-I$(RINCLUDES)

SOURCE_DIRS = src src/macclassic
CFLAGS		=
LDFLAGS		= 
LIBS		= -lm
OEXT    	= .elf
# performance too slow if not in release mode
RELEASE		= 1

TARGET		= ClassiCube-ppc
BUILD_DIR 	= build/mac_ppc

#---------------------------------------------------------------------------------
# executable generation
#---------------------------------------------------------------------------------
$(TARGET).bin $(TARGET).APPL $(TARGET).dsk: $(TARGET).pef
	$(REZ) $(REZFLAGS) \
		"misc/macclassic/ppcAPPL.r" \
		-t "APPL" -c "????" \
		--data $(TARGET).pef \
		-o $(TARGET).bin --cc $(TARGET).APPL --cc $(TARGET).dsk

$(TARGET).pef: $(TARGET).elf
	$(MakePEF) $(TARGET).elf -o $(TARGET).pef


include misc/makefiles/Makefile_common.mk
