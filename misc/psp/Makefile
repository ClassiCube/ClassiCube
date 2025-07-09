ifeq ($(strip $(PSPSDK)),)
$(warning "Please set PSPSDK variables in your environment. For example:")
$(warning   export PSPSDK=/usr/local/pspsk/psp/sdk)
$(warning   export PATH=/usr/local/pspsk/bin:$$PATH)
$(warning Or)
$(warning   export PSPSDK=$$(shell psp-config --pspsdk-path))
$(error   Failed to find PSPSDK installation)
endif


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
# Name of the final output
TARGET 			= ClassiCube-psp
# List of directories containing source code
SOURCE_DIRS		= src src/psp third_party/bearssl
# Directory where object files are placed
BUILD_DIR		= build/psp

PSP_EBOOT_TITLE  = ClassiCube
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

INCDIR   := $(PSPDEV)/psp/include $(PSPSDK)/include
LIBDIR   := $(PSPDEV)/psp/lib $(PSPSDK)/lib

CFLAGS   := $(addprefix -I,$(INCDIR)) -g -O1 -fno-math-errno -D_PSP_FW_VERSION=600
ASFLAGS  := $(addprefix -I,$(INCDIR)) -g

LDFLAGS  := $(addprefix -L,$(LIBDIR)) -specs=$(PSPSDK)/lib/prxspecs -Wl,-q,-T$(PSPSDK)/lib/linkfile.prx -Wl,-zmax-page-size=128
LIBS 	 := -lm -lpspgum -lpspgu -lpspge -lpspdisplay -lpspctrl -lpspdebug -lpspnet -lpspnet_apctl

# Dependency tracking
DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d
DEPFILES := $(OBJS:%.o=%.d)


#---------------------------------------------------------------------------------
# Compiler tools
#---------------------------------------------------------------------------------
CC       = psp-gcc
AS       = psp-gcc
LD       = psp-gcc
MKSFO    = mksfoex
PACK_PBP = pack-pbp
FIXUP    = psp-fixup-imports


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
	psp-prxgen $< $@

$(PSP_EBOOT_SFO): 
	$(MKSFO) -d MEMSIZE=1 '$(PSP_EBOOT_TITLE)' $@

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
	$(VITA_CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@
	
$(BUILD_DIR)/%.o : third_party/bearssl/%.c
	$(CC) $(CFLAGS) -c $< -o $@


#---------------------------------------------------------------------------------
# Dependency tracking
#---------------------------------------------------------------------------------
$(DEPFILES):

include $(wildcard $(DEPFILES))
