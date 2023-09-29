BUILD_DIR		:= build-dreamcast
SOURCE_DIRS		:= src third_party/bearssl/src

C_FILES := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
OBJS 	:= $(addprefix $(BUILD_DIR)/, $(notdir $(C_FILES:%.c=%.o)))
CFLAGS	:=-g -O1 -pipe -fno-math-errno -Ithird_party/bearssl/inc
LDFLAGS=-g
LIBS=-lm

TARGET := ClassiCube-dc

ifeq ($(strip $(KOS_BASE)),)
$(error "Please set KOS variables in your environment.")
endif

default: $(BUILD_DIR) $(TARGET).cdi

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: src/%.c
	kos-cc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: third_party/bearssl/src/%.c
	kos-cc $(CFLAGS) -c $< -o $@
	

$(TARGET).elf: $(OBJS)
	kos-cc $(LDFLAGS) $^ -o $@ $(LIBS)
	
$(TARGET).bin: $(TARGET).elf
	sh-elf-objcopy -R .stack -O binary $(TARGET).elf $(TARGET).bin
	
# https://dcemulation.org/phpBB/viewtopic.php?t=105269
$(TARGET)-scr.bin: $(TARGET).bin
	$(KOS_BASE)/utils/scramble/scramble $(TARGET).bin $(TARGET)-scr.bin
	
$(TARGET).iso: $(TARGET)-scr.bin
	mkdir -p ISO_FILES
	cp $(TARGET)-scr.bin ISO_FILES/1ST_READ.BIN
	cp misc/dreamcast/IP.BIN IP.BIN
	mkisofs -G IP.BIN -C 0,11702 -J -l -r -o $(TARGET).iso ISO_FILES
	# genisoimage -V ClassiCube -G IP.BIN -joliet -rock -l -o $(TARGET).iso ISO_FILES
	
$(TARGET).cdi: $(TARGET).iso
	cdi4dc $(TARGET).iso $(TARGET).cdi