ifeq ($(strip $(PSPDEV)),)
$(warning "Please set PSPDEV variables in your environment. For example:")
$(warning   export PSPDEV=/usr/local/pspsdk)
$(error   Failed to find PSPSDK installation)
endif
PSPSDK=$(PSPDEV)/psp/sdk

.SUFFIXES:


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
# Name of the final output
TARGET 			= ClassiCube-psp
# List of directories containing source code
SOURCE_DIRS		= src src/psp third_party/bearssl
# Directory where object files are placed
BUILD_DIR		= build/psp

SFO_TITLE  = ClassiCube
SFO_DISCID = CUBE02000

PSP_EBOOT_ICON	 = misc/psp/ICON0.png
PSP_EBOOT_SFO    = PARAM.SFO
PSP_EBOOT_ICON1  = NULL
PSP_EBOOT_UNKPNG = NULL
PSP_EBOOT_PIC1   = NULL
PSP_EBOOT_SND0   = NULL
PSP_EBOOT_PSAR   = NULL
PSP_EBOOT        = EBOOT.PBP


#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------
S_FILES := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.S))
C_FILES := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
OBJS 	:= $(addprefix $(BUILD_DIR)/, $(notdir $(C_FILES:%.c=%.o) $(S_FILES:%.S=%.o)))

CFLAGS   := -I$(PSPSDK)/include -g -O1 -fno-math-errno -D_PSP_FW_VERSION=600 -DPLAT_PSP
ASFLAGS  := -I$(PSPSDK)/include -g

LDFLAGS  := -L$(PSPSDK)/lib -specs=$(PSPSDK)/lib/prxspecs -Wl,-q,-T$(PSPSDK)/lib/linkfile.prx -Wl,-zmax-page-size=128
LIBS 	 := -lm -lpspgu -lpspge -lpspdisplay -lpspctrl -lpspnet -lpspnet_apctl

# Dependency tracking
DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d
DEPFILES := $(OBJS:%.o=%.d)


#---------------------------------------------------------------------------------
# Compiler tools
#---------------------------------------------------------------------------------
CC       = $(PSPDEV)/bin/psp-gcc
AS       = $(PSPDEV)/bin/psp-gcc
LD       = $(PSPDEV)/bin/psp-gcc
MKSFO    = $(PSPDEV)/bin/mksfoex
PACK_PBP = $(PSPDEV)/bin/pack-pbp
FIXUP    = $(PSPDEV)/bin/psp-fixup-imports
PRXGEN   = $(PSPDEV)/bin/psp-prxgen


#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all: $(BUILD_DIR) $(PSP_EBOOT)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean: 
	-rm -f $(TARGET).prx $(TARGET).elf $(OBJS) $(PSP_EBOOT_SFO) $(PSP_EBOOT)


#---------------------------------------------------------------------------------
# executable generation
#---------------------------------------------------------------------------------
$(TARGET).elf: $(OBJS)
	$(LD) $^ $(LDFLAGS) $(LIBS) -o $@
	$(FIXUP) $@

$(TARGET).prx: $(TARGET).elf
	$(PRXGEN) $< $@

$(PSP_EBOOT_SFO): 
	$(MKSFO) -d MEMSIZE=1 -s DISC_ID="$(SFO_DISCID)" '$(SFO_TITLE)' $@

$(PSP_EBOOT): $(TARGET).prx $(PSP_EBOOT_SFO)
	$(PACK_PBP) $(PSP_EBOOT) $(PSP_EBOOT_SFO) $(PSP_EBOOT_ICON)  \
		$(PSP_EBOOT_ICON1) $(PSP_EBOOT_UNKPNG) $(PSP_EBOOT_PIC1)  \
		$(PSP_EBOOT_SND0)  $(TARGET).prx $(PSP_EBOOT_PSAR)


#---------------------------------------------------------------------------------
# object generation
#---------------------------------------------------------------------------------
$(BUILD_DIR)/%.o : src/%.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o : src/psp/%.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o : src/psp/%.S
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@
	
$(BUILD_DIR)/%.o : third_party/bearssl/%.c
	$(CC) $(CFLAGS) -c $< -o $@


#---------------------------------------------------------------------------------
# Dependency tracking
#---------------------------------------------------------------------------------
$(DEPFILES):

include $(wildcard $(DEPFILES))
