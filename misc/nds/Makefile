# SPDX-License-Identifier: CC0-1.0
#
# SPDX-FileContributor: Antonio Niño Díaz, 2023
export BLOCKSDS			?= /opt/wonderful/thirdparty/blocksds/core
export BLOCKSDSEXT		?= /opt/wonderful/thirdparty/blocksds/external

GAME_TITLE	:= ClassiCube
GAME_SUBTITLE	:= Built with BlocksDS
GAME_AUTHOR	:= ClassiCube team
GAME_ICON	:= misc/nds/icon.bmp
GAME_FULL_TITLE := $(GAME_TITLE);$(GAME_SUBTITLE);$(GAME_AUTHOR)


# DLDI and internal SD slot of DSi
# --------------------------------

# Root folder of the SD image
SDROOT		:= sdroot
# Name of the generated image it "DSi-1.sd" for no$gba in DSi mode
SDIMAGE		:= image.bin

# Build artfacts
# --------------
ifdef BUILD_NONET
BUILDDIR	:= build/nds_nonet
ROM			:= ClassiCube-nonet.nds
else
BUILDDIR	:= build/nds
ROM			:= ClassiCube.nds
endif

# Targets
# -------
.PHONY: all clean arm9 arm7 dldipatch sdimage

all: $(ROM)

clean:
	$(MAKE) -f misc/nds/Makefile.arm9 clean --no-print-directory
	$(MAKE) -f misc/nds/Makefile.arm7 clean --no-print-directory
	$(RM) $(ROM) $(BUILDDIR) $(SDIMAGE)

arm9:
	$(MAKE) -f misc/nds/Makefile.arm9 --no-print-directory

arm7:
	$(MAKE) -f misc/nds/Makefile.arm7 --no-print-directory


$(ROM): arm9 arm7
	$(BLOCKSDS)/tools/ndstool/ndstool -c $@ \
		-7 $(BUILDDIR)/cc-arm7.elf -9 $(BUILDDIR)/cc-arm9.elf \
		-b $(GAME_ICON) "$(GAME_FULL_TITLE)"


sdimage:
	$(BLOCKSDS)/tools/mkfatimg/mkfatimg -t $(SDROOT) $(SDIMAGE)

dldipatch: $(ROM)
	$(BLOCKSDS)/tools/dlditool/dlditool $(BLOCKSDS)/tools/dldi/r4tfv2.dldi $(ROM)
