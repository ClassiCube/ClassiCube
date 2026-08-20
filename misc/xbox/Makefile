ifeq ($(strip $(NXDK_DIR)),)
$(error Please set NXDK_DIR in your environment. For example:)
$(warning /opt/nxdk/bin/activate)
$(error Undefined NXDK_DIR (root directory))
endif

.SUFFIXES:


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
TARGET		:= ClassiCube-xbox
SOURCE_DIRS := src src/xbox third_party/bearssl
BUILD_DIR   := build/xbox
RUN_PROGRAM	?= xemu

XBE_TITLE 	:= ClassiCube

default: $(TARGET).iso


#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------
CFLAGS := -O2 -fno-math-errno -Wno-builtin-macro-redefined \
	-I$(NXDK_DIR)/lib/net/lwip/src/include \
	-I$(NXDK_DIR)/lib/net/nforceif/include \
	-I$(NXDK_DIR)/lib/net/nvnetdrv \
	-I$(NXDK_DIR)/lib/usb/libusbohci/inc \
	-I$(NXDK_DIR)/lib/usb/libusbohci_xbox/ \
	-DUSBH_USE_EXTERNAL_CONFIG=\"usbh_config_xbox.h\"

LDFLAGS := -Wl,-stack:196608 \
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
CC 		:= $(NXDK_DIR)/bin/nxdk-cc
LINK	:= $(CC)

CGC := $(NXDK_DIR)/tools/cg/linux/cgc
# NOTE: Linux only. Would need changing for other platforms

CXBE         := $(NXDK_DIR)/tools/cxbe/cxbe
VP20COMPILER := $(NXDK_DIR)/tools/vp20compiler/vp20compiler
FP20COMPILER := $(NXDK_DIR)/tools/fp20compiler/fp20compiler
EXTRACT_XISO := $(NXDK_DIR)/tools/extract-xiso/build/extract-xiso


#---------------------------------------------------------------------------------
# shader object generation
#---------------------------------------------------------------------------------
SHDR_DIR	:= misc/xbox
SHDR_SRCS 	:= $(notdir $(wildcard $(SHDR_DIR)/*.cg))
SHDR_OBJS	:= $(addprefix $(BUILD_DIR)/, $(SHDR_SRCS:.cg=.inl))
GEN_FILES	+= $(SHDR_OBJS)

.PRECIOUS: $(SHDR_DIR)/vs_%.asm $(SHDR_DIR)/ps_%.asm
# prevent the .asm files from being deleted

$(BUILD_DIR)/vs_%.inl: $(SHDR_DIR)/vs_%.asm
	$(VP20COMPILER) $< > $@

$(SHDR_DIR)/vs_%.asm: $(SHDR_DIR)/vs_%.cg
	$(CGC) -profile vp20 $< -o $@

$(BUILD_DIR)/ps_%.inl: $(SHDR_DIR)/ps_%.asm
	$(FP20COMPILER) $< > $@

$(SHDR_DIR)/ps_%.asm: $(SHDR_DIR)/ps_%.cg
	$(CGC) -profile fp20 $< -o $@

src/xbox/Graphics_Xbox.c : $(SHDR_OBJS)


#---------------------------------------------------------------------------------
# Executable generation
#---------------------------------------------------------------------------------
OEXT		:= .exe
GEN_FILES 	:= $(TARGET).iso $(TARGET).xbe
include misc/makefiles/common_build.mk

$(TARGET).xbe: $(TARGET).exe
	$(CXBE) -OUT:$@ -TITLE:$(XBE_TITLE) $<

$(TARGET).iso: $(TARGET).xbe
	mkdir -p $(BUILD_DIR)/cd
	cp $(TARGET).xbe $(BUILD_DIR)/cd/default.xbe
	$(EXTRACT_XISO) -c $(BUILD_DIR)/cd $(XISO_FLAGS) $@


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk
