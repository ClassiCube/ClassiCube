#-------------------------------------
# Common configurable flags and names
#-------------------------------------
# Flags passed to the C compiler
CFLAGS  += -pipe -fno-math-errno -Werror -Wno-error=missing-braces -Wno-error=strict-aliasing
# Optimization level in release builds
OPT_LEVEL ?= 2
# Linker executable (overridable)
LINK ?= $(CC)
# Enables dependency tracking (https://make.mad-scientist.net/papers/advanced-auto-dependency-generation/)
# This ensures that changing a .h file automatically results in the .c files using it being auto recompiled when next running make
# Other compilers or on older systems the required GCC options may not be supported - if so, change TRACK_DEPENDENCIES to 0
TRACK_DEPENDENCIES ?= 1

#-----------------------------
# Autoconfigured variables
#-----------------------------
BUILD_DIRS	= $(BUILD_DIR) $(addprefix $(BUILD_DIR)/, $(SOURCE_DIRS))
C_SOURCES   = $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
C_OBJECTS   = $(patsubst %.c, $(BUILD_DIR)/%.o, $(C_SOURCES))
OBJECTS += $(C_OBJECTS)


#----------------------------------------------------------------
# Determine shell command used to remove files (for "make clean")
#----------------------------------------------------------------
ifndef RM
	# No prefined RM variable, try to guess OS default
	ifeq ($(OS),Windows_NT)
		RM = del
	else
		RM = rm -f
	endif
endif


#----------------------------------------------------------------
# Platform independent flags
#----------------------------------------------------------------
ifdef BUILD_SDL2
	CFLAGS += -DCC_WIN_BACKEND=CC_WIN_BACKEND_SDL2
	LIBS += -lSDL2
endif
ifdef BUILD_SDL3
	CFLAGS += -DCC_WIN_BACKEND=CC_WIN_BACKEND_SDL3
	LIBS += -lSDL3
endif
ifdef BUILD_TERMINAL
	CFLAGS += -DCC_WIN_BACKEND=CC_WIN_BACKEND_TERMINAL -DCC_GFX_BACKEND=CC_GFX_BACKEND_SOFTGPU
	LIBS := $(subst mwindows,mconsole,$(LIBS))
endif

ifdef RELEASE
	CFLAGS += -O$(OPT_LEVEL)
else
	CFLAGS += -g
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

# Cleans up all build .o files
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
