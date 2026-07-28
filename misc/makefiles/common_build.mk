# Enables dependency tracking (https://make.mad-scientist.net/papers/advanced-auto-dependency-generation/)
# This ensures that changing a .h file automatically results in the .c files using it being auto recompiled when next running make
# Other compilers or on older systems the required GCC options may not be supported - if so, set TRACK_DEPENDENCIES to 0 beforehand
TRACK_DEPENDENCIES ?= 1

#-----------------------------
# Source file gathering
#-----------------------------
C_SOURCES	:= $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
CPP_SOURCES	:= $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.cpp))
S_SOURCES	:= $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.S))
M_SOURCES	:= $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.m))

#-----------------------------
# Autoconfigured variables
#-----------------------------
BUILD_ROOT := $(BUILD_DIR)
ifdef BUILD_ARCH
BUILD_ROOT := $(BUILD_ROOT)/$(BUILD_ARCH)
endif

C_OBJECTS	:= $(patsubst %.c,$(BUILD_ROOT)/%.o, $(C_SOURCES))
CPP_OBJECTS	:= $(patsubst %.cpp,$(BUILD_ROOT)/%.o, $(CPP_SOURCES))
S_OBJECTS	:= $(patsubst %.S,$(BUILD_ROOT)/%.o, $(S_SOURCES))
M_OBJECTS	:= $(patsubst %.m,$(BUILD_ROOT)/%.o, $(M_SOURCES))

BUILD_DIRS	:= $(BUILD_ROOT) $(addprefix $(BUILD_ROOT)/, $(SOURCE_DIRS))
OBJECTS		+= $(C_OBJECTS) $(CPP_OBJECTS) $(S_OBJECTS) $(M_OBJECTS)
# Additional generated files that are cleaned up by 'clean' target
GEN_FILES	+= $(TARGET)$(OEXT)


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
$(TARGET)$(OEXT): $(OBJECTS) $(BUILD_DIRS)
	$(LINK) $(LDFLAGS) $(EXTRA_LDFLAGS) -o $@ $(OBJECTS) $(LIBS) $(EXTRA_LIBS)
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
	$(RM) $(GEN_FILES) $(OBJECTS) $(DEPFILES)


#------------------------------------------------
# Dependency tracking + object compilation
#------------------------------------------------
ifeq ($(TRACK_DEPENDENCIES), 1)
# === Compiling with dependency tracking ===
DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILD_ROOT)/$*.d
DEPFILES := $(patsubst %.o,%.d, $(OBJECTS))
$(DEPFILES):

$(BUILD_ROOT)/%.o : %.c $(BUILD_ROOT)/%.d
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(DEPFLAGS) -c $< -o $@
$(BUILD_ROOT)/%.o : %.S $(BUILD_ROOT)/%.d
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(DEPFLAGS) -c $< -o $@
$(BUILD_ROOT)/%.o : %.cpp $(BUILD_ROOT)/%.d
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(DEPFLAGS) -c $< -o $@
$(BUILD_ROOT)/%.o : %.m $(BUILD_ROOT)/%.d
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(DEPFLAGS) -c $< -o $@
	
include $(wildcard $(DEPFILES))

else
# === Compiling without dependency tracking ===
$(BUILD_ROOT)/%.o : %.c
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@
$(BUILD_ROOT)/%.o : %.S
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@
$(BUILD_ROOT)/%.o : %.cpp
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@
$(BUILD_ROOT)/%.o : %.m
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@

# Assume worst case scenario and recompile everything if a .h changes
H_FILES := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.h))
$(OBJECTS) : $(H_FILES)

endif
