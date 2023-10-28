all: ClassiCube-n64.z64

BUILD_DIR = build-n64
SOURCE_DIR = src
N64_ROM_TITLE = "ClassiCube"
N64_ROM_RTC = true

include $(N64_INST)/include/n64.mk

CFILES 	:= $(notdir $(wildcard src/*.c))
OFILES 	:= $(CFILES:.c=.o)
OBJS 	:= $(addprefix $(BUILD_DIR)/,$(OFILES))
CFLAGS	:= "-DNINTENDO64 -Wno-error=missing-braces"

ClassiCube-n64.z64: N64_ROM_TITLE = "ClassiCube"

$(BUILD_DIR)/ClassiCube-n64.elf: $(OBJS)

clean:
	rm -rf $(BUILD_DIR) *.z64
.PHONY: clean

-include $(wildcard $(BUILD_DIR)/*.d)
