BUILD_DIR		= build-n64
SOURCE_DIR	 	= src
N64_ROM_TITLE 	= "ClassiCube"
N64_ROM_RTC 	= true
TARGET			= ClassiCube-n64
N64_MKDFS_ROOT	= "misc/n64"

CFILES 	:= $(notdir $(wildcard src/*.c))
OFILES 	:= $(CFILES:.c=.o)
OBJS 	:= $(addprefix $(BUILD_DIR)/,$(OFILES))
CFLAGS	:= -Wno-error=missing-braces -Wno-error=strict-aliasing -Wno-error=incompatible-pointer-types

default: $(TARGET).z64

include $(N64_INST)/include/n64.mk

$(TARGET).z64: N64_ROM_TITLE = "ClassiCube"
$(TARGET).z64: $(BUILD_DIR)/filesystem.dfs

$(BUILD_DIR)/filesystem.dfs: misc/n64/default.zip

$(BUILD_DIR)/ClassiCube-n64.elf: $(OBJS)

clean:
	rm -rf $(BUILD_DIR) $(TARGET).z64
.PHONY: clean

-include $(wildcard $(BUILD_DIR)/*.d)
