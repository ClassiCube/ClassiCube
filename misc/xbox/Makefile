ifeq ($(strip $(NXDK_DIR)),)
$(error "Please set NXDK_DIR in your environment")
endif

.SUFFIXES:


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
# Name of the final output
TARGET		= ClassiCube-xbox
# Application name/title
XBE_TITLE 	= ClassiCube
# List of directories containing source code
SOURCE_DIRS = src src/xbox third_party/bearssl
# Shader objects
SHADER_OBJS = misc/xbox/vs_coloured.inl misc/xbox/vs_textured.inl misc/xbox/vs_offset.inl misc/xbox/ps_coloured.inl misc/xbox/ps_textured.inl
# Directory where object files are placed
BUILD_DIR   = build/xbox


#---------------------------------------------------------------------------------
# Compilable files
#---------------------------------------------------------------------------------
S_FILES = $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.S))
C_FILES = $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
OBJS 	= $(addprefix $(BUILD_DIR)/, $(notdir $(C_FILES:%.c=%.o) $(S_FILES:%.S=%.o)))

# Dependency tracking
DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d
DEPFILES := $(OBJS:%.o=%.d)


#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------
CFLAGS = -O1 -fno-math-errno -Wno-builtin-macro-redefined \
	-I$(NXDK_DIR)/lib/net/lwip/src/include \
	-I$(NXDK_DIR)/lib/net/nforceif/include \
	-I$(NXDK_DIR)/lib/net/nvnetdrv \
	-I$(NXDK_DIR)/lib/usb/libusbohci/inc \
	-I$(NXDK_DIR)/lib/usb/libusbohci_xbox/ \
	-DUSBH_USE_EXTERNAL_CONFIG=\"usbh_config_xbox.h\"

LDFLAGS = -stack:196608 \
	$(NXDK_DIR)/lib/libnxdk.lib \
	$(NXDK_DIR)/lib/libnxdk_hal.lib \
	$(NXDK_DIR)/lib/libnxdk_net.lib \
	$(NXDK_DIR)/lib/libpbkit.lib \
	$(NXDK_DIR)/lib/libpdclib.lib \
	$(NXDK_DIR)/lib/libxboxrt.lib \
	$(NXDK_DIR)/lib/libwinapi.lib \
	$(NXDK_DIR)/lib/nxdk_usb.lib \
	$(NXDK_DIR)/lib/xboxkrnl/libxboxkrnl.lib


#---------------------------------------------------------------------------------
# Tools
#---------------------------------------------------------------------------------
CGC = $(NXDK_DIR)/tools/cg/linux/cgc
# NOTE: Linux only. Would need changing for other platforms

CXBE         = $(NXDK_DIR)/tools/cxbe/cxbe
VP20COMPILER = $(NXDK_DIR)/tools/vp20compiler/vp20compiler
FP20COMPILER = $(NXDK_DIR)/tools/fp20compiler/fp20compiler
EXTRACT_XISO = $(NXDK_DIR)/tools/extract-xiso/build/extract-xiso


#---------------------------------------------------------------------------------
# Main targets
#---------------------------------------------------------------------------------
default: $(BUILD_DIR) $(TARGET).iso

clean:
	rm -f $(TARGET).iso $(TARGET).xbe $(TARGET).exe $(OBJS) $(SHADER_OBJS) $(DEPFILES)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)


#---------------------------------------------------------------------------------
# Executable generation
#---------------------------------------------------------------------------------
$(BUILD_DIR)/Graphics_Xbox.o : $(SHADER_OBJS)

$(TARGET).iso: $(TARGET).xbe
	mkdir -p $(BUILD_DIR)/cd
	cp $(TARGET).xbe $(BUILD_DIR)/cd/default.xbe
	$(EXTRACT_XISO) -c $(BUILD_DIR)/cd $(XISO_FLAGS) $@

$(TARGET).xbe: $(TARGET).exe
	$(CXBE) -OUT:$@ -TITLE:$(XBE_TITLE) $<

$(TARGET).exe: $(OBJS)
	nxdk-link $(NXDK_LDFLAGS) $(LDFLAGS) -out:$@ $^


#---------------------------------------------------------------------------------
# Object generation
#---------------------------------------------------------------------------------
$(BUILD_DIR)/%.o: src/%.c
	nxdk-cc $(NXDK_CFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: src/xbox/%.c
	nxdk-cc $(NXDK_CFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: src/xbox/%.S
	nxdk-as $(NXDK_ASFLAGS) $(ASFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: third_party/bearssl/%.c
	nxdk-cc $(NXDK_CFLAGS) $(CFLAGS) -c $< -o $@


# prevent the .asm files from being deleted
.PRECIOUS: vs_%.asm ps_%.asm

vs_%.inl: vs_%.asm
	$(VP20COMPILER) $< > $@

vs_%.asm: vs_%.cg
	$(CGC) -profile vp20 $< -o $@

ps_%.inl: ps_%.asm
	$(FP20COMPILER) $< > $@

ps_%.asm: ps_%.cg
	$(CGC) -profile fp20 $< -o $@


#---------------------------------------------------------------------------------
# Dependency tracking
#---------------------------------------------------------------------------------
$(DEPFILES):

include $(wildcard $(DEPFILES))
