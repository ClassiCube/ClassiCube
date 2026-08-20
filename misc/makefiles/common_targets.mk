.PHONY: clean run

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
# Misc targets section
#------------------------------------------------
# Cleans up all built files
clean:
	$(RM) $(GEN_FILES)

# Runs the main executable
run: $(TARGET)$(OEXT)
	$(RUN_PROGRAM) ./$(TARGET)$(OEXT)

