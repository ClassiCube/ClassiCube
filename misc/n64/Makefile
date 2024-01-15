all: ClassiCube-n64.z64

BUILD_DIR = build-n64
SOURCE_DIR = src
N64_ROM_TITLE = "ClassiCube"
N64_ROM_RTC = true

CFILES 	:= $(notdir $(wildcard src/*.c))
OFILES 	:= $(CFILES:.c=.o)
OBJS 	:= $(addprefix $(BUILD_DIR)/,$(OFILES))
U64FLG	:=  "-DFLAT_GEN" "-DR480i"
CFLAGS	:= "-DULTRA64" $(U64FLG) "-Wno-error=pointer-sign" "-Wno-error=return-type" "-Wno-error=missing-braces" "-Wno-error=incompatible-pointer-types" "-Wno-error=implicit-function-declaration" "-Wno-error=strict-aliasing" "-Wno-error=maybe-uninitialized"

#	Digi-Space Productions comment on 2:34P 1/14/2024:
#
#	I will try to add some defines for stuff like Texture Filtering, Resolution, and different input mappings.
#	Might also get a friend of mine to make a few tracks for the 64 port, since I can use XM files.
#

#	Defines:
#
#	ULTRA64:	Just makes it so Ultra64/N64 specific code blocks can be enabled/disabled
#	TFILTER:	Enable the Trilinear/3-Point texture filter.
#	R480i:		Enable 640x480i resolution.
#	FLAT_GEN:	Generate with Flat terrain
#
#
#

include $(N64_INST)/include/n64.mk

ClassiCube-n64.z64: N64_ROM_TITLE = "ClassiCube"
ClassiCube-n64.z64: $(BUILD_DIR)/filesystem.dfs

$(BUILD_DIR)/filesystem.dfs: misc/n64/default.zip

$(BUILD_DIR)/ClassiCube-n64.elf: $(OBJS)

clean:
	rm -rf $(BUILD_DIR) *.z64
	rm -rf $(BUILD_DIR)/build-n64
.PHONY: clean

-include $(wildcard $(BUILD_DIR)/*.d)
