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
EXE_OBJECTS := $(C_OBJECTS) $(CPP_OBJECTS) $(S_OBJECTS) $(M_OBJECTS) $(OBJECTS)
# Additional generated files that are cleaned up by 'clean' target
GEN_FILES	:= $(GEN_FILES) $(TARGET)$(OEXT) $(EXE_OBJECTS)


#------------------------------------------------
# Main executable compilation
#------------------------------------------------
$(TARGET)$(OEXT): $(EXE_OBJECTS)
	$(LINK) $(LDFLAGS) $(EXTRA_LDFLAGS) $^ -o $@ $(LIBS) $(EXTRA_LIBS)
	@echo "----------------------------------------------------"
	@echo "Successfully compiled executable: $@"
	@echo "----------------------------------------------------"

# ensure correct linker is used when this file is included multiple times
$(TARGET)$(OEXT): LINK:=$(LINK)

# build directories are an order only pre-requisite
$(EXE_OBJECTS): | $(BUILD_DIRS)

# Auto creates directories for build files (.o and .d files)
$(BUILD_DIRS):
	mkdir -p $@


#------------------------------------------------
# Dependency tracking + object compilation
#------------------------------------------------
ifeq ($(TRACK_DEPENDENCIES), 1)
# === Compiling with dependency tracking ===
DEPFILES 	:= $(patsubst %.o,%.d,$(EXE_OBJECTS))
GEN_FILES	:= $(GEN_FILES) $(DEPFILES)
$(DEPFILES):

$(BUILD_ROOT)/%.o : %.c $(BUILD_ROOT)/%.d
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@ -MT $@ -MMD -MP -MF $(patsubst %.o,%.d,$@)
$(BUILD_ROOT)/%.o : %.S $(BUILD_ROOT)/%.d
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@ -MT $@ -MMD -MP -MF $(patsubst %.o,%.d,$@)
$(BUILD_ROOT)/%.o : %.cpp $(BUILD_ROOT)/%.d
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@ -MT $@ -MMD -MP -MF $(patsubst %.o,%.d,$@)
$(BUILD_ROOT)/%.o : %.m $(BUILD_ROOT)/%.d
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@ -MT $@ -MMD -MP -MF $(patsubst %.o,%.d,$@)

# ensure correct compiler is used when this file is included multiple times
$(BUILD_ROOT)/%.o: CC:=$(CC)
	
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
$(EXE_OBJECTS): $(H_FILES)

endif
