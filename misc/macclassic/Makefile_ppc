# path to RETRO68
RETRO68=../Retro68-build/toolchain

PREFIX=$(RETRO68)/powerpc-apple-macos
CC=$(RETRO68)/bin/powerpc-apple-macos-gcc
CXX=$(RETRO68)/bin/powerpc-apple-macos-g++
CFLAGS=-O1 -fno-math-errno

REZ=$(RETRO68)/bin/Rez
MakePEF=$(RETRO68)/bin/MakePEF

LDFLAGS=-lm
RINCLUDES=$(PREFIX)/RIncludes
REZFLAGS=-I$(RINCLUDES)

TARGET		:=	ClassiCube-ppc
BUILD_DIR 	:=	build/mac_ppc
SOURCE_DIR	:=	src
C_SOURCES   := $(wildcard $(SOURCE_DIR)/*.c)
C_OBJECTS   := $(patsubst $(SOURCE_DIR)/%.c, $(BUILD_DIR)/%.o, $(C_SOURCES))

# Dependency tracking
DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d
DEPFILES := $(C_OBJECTS:%.o=%.d)


#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
default: $(BUILD_DIR) $(TARGET).bin

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)


#---------------------------------------------------------------------------------
# executable generation
#---------------------------------------------------------------------------------
$(TARGET).bin $(TARGET).APPL $(TARGET).dsk: $(TARGET).pef
	$(REZ) $(REZFLAGS) \
		"misc/macclassic/ppcAPPL.r" \
		-t "APPL" -c "????" \
		--data $(TARGET).pef \
		-o $(TARGET).bin --cc $(TARGET).APPL --cc $(TARGET).dsk

$(TARGET).elf: $(C_OBJECTS)
	$(CC) $(C_OBJECTS) -o $@ $(LDFLAGS)

$(TARGET).pef: $(TARGET).elf
	$(MakePEF) $(TARGET).elf -o $(TARGET).pef

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)


#---------------------------------------------------------------------------------
# object generation
#---------------------------------------------------------------------------------
$(C_OBJECTS): $(BUILD_DIR)/%.o : $(SOURCE_DIR)/%.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

# Dependency tracking
$(DEPFILES):

include $(wildcard $(DEPFILES))
