# Enables dependency tracking (https://make.mad-scientist.net/papers/advanced-auto-dependency-generation/)
# This ensures that changing a .h file automatically results in the .c files using it being auto recompiled when next running make
# Other compilers or on older systems the required GCC options may not be supported - if so, set TRACK_DEPENDENCIES to 0 beforehand
TRACK_DEPENDENCIES ?= 1

#-----------------------------
# Autoconfigured variables
#-----------------------------
BUILD_DIRS	:= $(BUILD_DIR) $(addprefix $(BUILD_DIR)/, $(SOURCE_DIRS))
C_SOURCES   := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
C_OBJECTS   := $(patsubst %.c, $(BUILD_DIR)/%.o, $(C_SOURCES))
OBJECTS 	+= $(C_OBJECTS)


#----------------------------------------------------------------
# Determine shell command used to remove files (for "make clean")
#----------------------------------------------------------------
ifndef RM
	# No prefined RM variable, try to guess OS default
	ifeq ($(OS),Windows_NT)
		RM := del
	else
		RM := rm -f
	endif
endif


#------------------------------------------------
# Main executable compilation
#------------------------------------------------
$(TARGET)$(OEXT): $(BUILD_DIRS) $(OBJECTS)
	$(LINK) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS) $(EXTRA_LIBS)
	@echo "----------------------------------------------------"
	@echo "Successfully compiled executable: $@"
	@echo "----------------------------------------------------"


#------------------------------------------------
# Misc targets section
#------------------------------------------------
# Auto creates directories for build files (.o and .d files)
$(BUILD_DIRS):
	mkdir -p $@

# Cleans up all built files
clean:
	$(RM) $(OBJECTS)


#------------------------------------------------
# Dependency tracking + object compilation
#------------------------------------------------
ifeq ($(TRACK_DEPENDENCIES), 1)
# === Compiling with dependency tracking ===
DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d
DEPFILES := $(patsubst %.o, %.d, $(OBJECTS))
$(DEPFILES):

$(BUILD_DIR)/%.o : %.c $(BUILD_DIR)/%.d
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(DEPFLAGS) -c $< -o $@
$(BUILD_DIR)/%.o : %.cpp $(BUILD_DIR)/%.d
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(DEPFLAGS) -c $< -o $@
$(BUILD_DIR)/%.o : %.m $(BUILD_DIR)/%.d
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(DEPFLAGS) -c $< -o $@

include $(wildcard $(DEPFILES))

else
# === Compiling without dependency tracking ===
$(BUILD_DIR)/%.o : %.c
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@
$(BUILD_DIR)/%.o : %.cpp
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@
$(BUILD_DIR)/%.o : %.m
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@
endif
