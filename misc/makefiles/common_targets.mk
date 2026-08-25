.PHONY: clean run

#----------------------------------------------------------------
# Determine appropriate shell commands for filesystem operations
#----------------------------------------------------------------
# NOTE: msys treated the same as non-windows systems

ifneq ($(OS),Windows_NT)
    DEL_FILES = rm -f $(1)
else ifneq ($(strip $(MSYS)),)
    DEL_FILES = rm -f $(1)
else
    DEL_FILES = del $(subst /,\,$(1))
endif

ifneq ($(OS),Windows_NT)
    MAKE_DIR = mkdir -p $(1)
else ifneq ($(strip $(MSYS)),)
    MAKE_DIR = mkdir -p $(1)
else
    MAKE_DIR = mkdir $(subst /,\,$(1))
endif


#------------------------------------------------
# Misc targets section
#------------------------------------------------
# Cleans up all built files
clean:
	$(call DEL_FILES,$(GEN_FILES))

# Runs the main executable
run: $(TARGET)$(OEXT)
	$(RUN_PROGRAM) ./$(TARGET)$(OEXT)

