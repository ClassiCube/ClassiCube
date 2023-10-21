BUILD_DIR		:= build-dc
SOURCE_DIRS		:= src third_party/bearssl/src

C_FILES := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
OBJS 	:= $(addprefix $(BUILD_DIR)/, $(notdir $(C_FILES:%.c=%.o)))
CFLAGS	:=-g -O1 -pipe -fno-math-errno -Ithird_party/bearssl/inc

GLDC_LIB=third_party/gldc/libGLdc.a
LDFLAGS=-g
LIBS=-lm $(GLDC_LIB)

TARGET := ClassiCube-dc
CC_TEXTURES = classicube.zip

ifeq ($(strip $(KOS_BASE)),)
$(error "Please set KOS variables in your environment.")
endif

default: $(CC_TEXTURES) $(GLDC_LIB) $(BUILD_DIR) $(TARGET).cdi

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	
$(GLDC_LIB):
	$(MAKE) -C third_party/gldc

# TODO add textures to misc folder ?	
$(CC_TEXTURES): 
	curl http://www.classicube.net/static/default.zip -o $@
	
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
	mkdir -p ISO_FILES/texpacks
	cp $(CC_TEXTURES) ISO_FILES/texpacks/classicube.zip
	cp misc/dreamcast/IP.BIN IP.BIN
	mkisofs -G IP.BIN -C 0,11702 -J -l -r -o $(TARGET).iso ISO_FILES
	# genisoimage -V ClassiCube -G IP.BIN -joliet -rock -l -o $(TARGET).iso ISO_FILES
	
$(TARGET).cdi: $(TARGET).iso
	cdi4dc $(TARGET).iso $(TARGET).cdi