TARGET 			= ClassiCube-psp
PSP_EBOOT_TITLE = ClassiCube
PSP_EBOOT_ICON	= misc/psp/ICON0.png

BUILD_DIR		:= build-psp
SOURCE_DIRS		:= src third_party/bearssl/src

C_FILES := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
OBJS 	:= $(addprefix $(BUILD_DIR)/, $(notdir $(C_FILES:%.c=%.o)))

INCDIR   = third_party/bearssl/inc
CFLAGS   = -g -O1 -fno-math-errno
ASFLAGS  = $(CFLAGS)

LIBDIR  =
LDFLAGS =
LIBS    = -lm -lpspgum -lpspgu -lpspge -lpspdisplay -lpspctrl

# Dependency tracking
DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d
DEPFILES := $(OBJS:%.o=%.d)

BUILD_PRX 		= 1
EXTRA_TARGETS   = $(BUILD_DIR) EBOOT.PBP

ifeq ($(strip $(PSPSDK)),)
$(warning "Please set PSPSDK variables in your environment. For example:")
$(warning   export PSPSDK=/usr/local/pspsk/psp/sdk)
$(warning   export PATH=/usr/local/pspsk/bin:$$PATH)
$(warning Or)
$(warning   export PSPSDK=$$(shell psp-config --pspsdk-path))
$(error   Failed to find PSPSDK installation)
endif

include $(PSPSDK)/lib/build.mak


#---------------------------------------------------------------------------------
# object generation
#---------------------------------------------------------------------------------
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o : src/%.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@
	
$(BUILD_DIR)/%.o : third_party/bearssl/src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Dependency tracking
$(DEPFILES):

include $(wildcard $(DEPFILES))
