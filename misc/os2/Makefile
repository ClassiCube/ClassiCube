TARGET		:=	ClassiCube
BUILD_DIR 	:=	build/os2
SOURCE_DIR	:=	src
BEARSSL		:=	1

C_SOURCES   := $(wildcard $(SOURCE_DIR)/*.c)
ifeq ($(BEARSSL),1)
	C_SOURCES  += $(wildcard third_party/bearssl/*.c)
endif
C_OBJECTS   := $(patsubst $(SOURCE_DIR)/%.c, $(BUILD_DIR)/%.o, $(C_SOURCES))
OBJECTS 	:= $(C_OBJECTS) $(BUILD_DIR)/$(TARGET).res misc/os2/classicube.def

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
CC      := gcc
CFLAGS	:= -pipe -Wno-attributes -fno-math-errno -O3 -g -mtune=pentium4 -msse2 -march=i686 -idirafter /@unixroot/usr/include/os2tk45 -DOS2
LDFLAGS	:= -Zhigh-mem -Zomf -Zargs-wild -Zargs-resp -Zlinker DISABLE -Zlinker 1121
LIBS	:= -lcx -lmmpm2 -lpthread

default: $(TARGET).exe

$(TARGET).exe: $(BUILD_DIR) $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o : $(SOURCE_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(TARGET).res: misc/os2/$(TARGET).rc misc/os2/$(TARGET).ico
	wrc -r misc/os2/$(TARGET).rc -fo=$@

