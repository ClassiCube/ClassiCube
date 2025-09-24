CC=i586-pc-msdosdjgpp-gcc

CFLAGS =-O1 -fno-math-errno -Werror -Wno-error=missing-braces -Wno-error=strict-aliasing
LDFLAGS=-g

TARGET		:= CCDOS
BUILD_DIR 	:= build/msdos
SOURCE_DIRS	:= src src/msdos

C_FILES := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
OBJS 	:= $(addprefix $(BUILD_DIR)/, $(notdir $(C_FILES:%.c=%.o)))

# Dependency tracking
DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d
DEPFILES := $(C_OBJECTS:%.o=%.d)


#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
default: $(BUILD_DIR) $(TARGET).EXE

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)


#---------------------------------------------------------------------------------
# executable generation
#---------------------------------------------------------------------------------
$(TARGET).EXE: $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)


#---------------------------------------------------------------------------------
# object generation
#---------------------------------------------------------------------------------
$(BUILD_DIR)/%.o : src/%.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o : src/msdos/%.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

# Dependency tracking
$(DEPFILES):

include $(wildcard $(DEPFILES))
