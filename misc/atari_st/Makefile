TARGET		:=	ClassiCube-atari
BUILD_DIR 	:=	build/atari_st
SOURCE_DIRS	:= src src/atari_st

S_FILES := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.S))
C_FILES := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
OBJS 	:= $(addprefix $(BUILD_DIR)/, $(notdir $(C_FILES:%.c=%.o) $(S_FILES:%.S=%.o)))


#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------
CC = vc +tos
CFLAGS = -O1 -DPLAT_ATARIOS

AS = vasmm68k_mot
ASFLAGS = -Fvobj -m68000 -no-fpu

LDFLAGS = $(CFLAGS)
LDLIBS  = -lm


#---------------------------------------------------------------------------------
# Main targets
#---------------------------------------------------------------------------------
default: $(BUILD_DIR) $(TARGET).tos

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET).tos: $(OBJS)
	$(CC) $(LDFLAGS) -final -o $@ $^ $(LDLIBS)


#---------------------------------------------------------------------------------
# object generation
#---------------------------------------------------------------------------------
$(BUILD_DIR)/%.o : src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/%.o : src/atari_st/%.c
	$(CC) $(CFLAGS) -c -o $@ $<
