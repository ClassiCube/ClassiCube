# path to RETRO68
RETRO68=../Retro68-build/toolchain

PREFIX=$(RETRO68)/m68k-apple-macos
CC=$(RETRO68)/bin/m68k-apple-macos-gcc
CXX=$(RETRO68)/bin/m68k-apple-macos-g++

REZ=$(RETRO68)/bin/Rez
RINCLUDES=$(PREFIX)/RIncludes
REZFLAGS=-I$(RINCLUDES)

SOURCE_DIRS = src src/macclassic
LDFLAGS		= 
LIBS		= -lm
OEXT    	= .code.bin
# performance too slow if not in release mode
RELEASE		= 1

ifdef ARCH_68040
	TARGET		= ClassiCube-68040
	BUILD_DIR 	= build/mac_68040
	CFLAGS		= -march=68040
else
	TARGET		= ClassiCube-68k
	BUILD_DIR 	= build/mac_68k
	CFLAGS		= -DCC_BUILD_FPU_MODE=CC_FPU_MODE_MINIMAL -DCC_BUILD_TINYMEM -DCC_GFX_BACKEND=CC_GFX_BACKEND_SOFTMIN
endif

#---------------------------------------------------------------------------------
# executable generation
#---------------------------------------------------------------------------------
$(TARGET).bin $(TARGET).APPL $(TARGET).dsk: $(TARGET).code.bin
	$(REZ) $(REZFLAGS) \
		--copy "$(TARGET).code.bin" \
		"misc/macclassic/68APPL.r" \
		-t "APPL" -c "????" \
		-o $(TARGET).bin --cc $(TARGET).APPL --cc $(TARGET).dsk


include misc/makefiles/Makefile_common.mk
