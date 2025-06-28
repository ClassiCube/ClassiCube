ifeq ($(strip $(NXDK_DIR)),)
$(error "Please set NXDK_DIR in your environment")
endif

#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
TARGET		= ClassiCube-xbox
XBE_TITLE 	= ClassiCube
SOURCE_DIRS = src src/xbox third_party/bearssl/src
SHADER_OBJS = misc/xbox/vs_coloured.inl misc/xbox/vs_textured.inl misc/xbox/ps_coloured.inl misc/xbox/ps_textured.inl
BUILD_DIR   = build/xbox


#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------
S_FILES = $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.S))
C_FILES = $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
OBJS 	= $(addprefix $(BUILD_DIR)/, $(notdir $(C_FILES:%.c=%.o) $(S_FILES:%.S=%.o)))

CFLAGS = -Ithird_party/bearssl/inc -O1 -fno-math-errno -Wno-builtin-macro-redefined \
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

# Dependency tracking
DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d
DEPFILES := $(OBJS:%.o=%.d)


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
$(OBJS) : $(SHADER_OBJS)

$(TARGET).iso: $(TARGET).xbe
	mkdir -p $(BUILD_DIR)/cd
	cp $(TARGET).xbe $(BUILD_DIR)/cd/default.xbe
	$(EXTRACT_XISO) -c $(BUILD_DIR)/cd $(XISO_FLAGS) $@

$(TARGET).xbe: $(TARGET).exe
	$(CXBE) -OUT:$@ -TITLE:$(XBE_TITLE) $<

$(TARGET).exe : $(OBJS)
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

$(BUILD_DIR)/%.o: third_party/bearssl/src/%.c
	nxdk-cc $(NXDK_CFLAGS) $(CFLAGS) -c $< -o $@

%.inl: %.vs.cg
	$(CGC) -profile vp20 -o $@.$$$$ $< && \
	$(VP20COMPILER) $@.$$$$ > $@ && \
	rm -rf $@.$$$$

%.inl: %.ps.cg
	$(CGC) -profile fp20 -o $@.$$$$ $< && \
	$(FP20COMPILER) $@.$$$$ > $@ && \
	rm -rf $@.$$$$


#---------------------------------------------------------------------------------
# Dependency tracking
#---------------------------------------------------------------------------------
$(DEPFILES):

include $(wildcard $(DEPFILES))
