BUILD_DIR		:= build-dreamcast
SOURCE_DIRS		:= src third_party/bearssl/src

C_FILES := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
OBJS 	:= $(addprefix $(BUILD_DIR)/, $(notdir $(C_FILES:%.c=%.o)))
CFLAGS	:=-g -O1 -pipe -fno-math-errno

TARGET := ClassiCube-dc

ifeq ($(strip $(KOS_BASE)),)
$(error "Please set KOS variables in your environment.")
endif

default: $(BUILD_DIR) $(TARGET)-scr.bin
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: src/%.c
	kos-cc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: third_party/bearssl/src/%.c
	kos-cc $(CFLAGS) -c $< -o $@
	

$(TARGET).elf: $(OBJS)
	kos-cc $< -o $@
	
$(TARGET).bin: $(TARGET).elf
	sh-elf-objcopy -R .stack -O binary $(TARGET).elf $(TARGET).bin
	
# https://dcemulation.org/phpBB/viewtopic.php?t=105269
$(TARGET)-scr.bin: $(TARGET).bin
	$(KOS_BASE)/utils/scramble/scramble $(TARGET).bin $(TARGET)-scr.bin