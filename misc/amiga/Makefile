TARGET		:=	ClassiCube-amiga
BUILD_DIR 	:=	build/amiga
SOURCE_DIRS	:= src src/amiga

S_FILES := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.S))
C_FILES := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
OBJS 	:= $(addprefix $(BUILD_DIR)/, $(notdir $(C_FILES:%.c=%.o) $(S_FILES:%.S=%.o)))


#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------
CC = vc +aos68k
CFLAGS = -rmcfg-amiga-softfloat -O1 -DPLAT_AMIGA

AS = vasmm68k_mot
ASFLAGS = -Fvobj -m68000 -no-fpu

LDFLAGS = $(CFLAGS)
LDLIBS  = -lamiga -lmsoft


#---------------------------------------------------------------------------------
# Main targets
#---------------------------------------------------------------------------------
default: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -final -o $@ $^ $(LDLIBS)


#---------------------------------------------------------------------------------
# object generation
#---------------------------------------------------------------------------------
$(BUILD_DIR)/%.o : src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/%.o : src/amiga/%.c
	$(CC) $(CFLAGS) -c -o $@ $<
