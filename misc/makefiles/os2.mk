BUILD_DIR 	:= build/os2
SOURCE_DIRS	:= src src/os2 third_party/bearssl

include misc/makefiles/common_config.mk
CFLAGS	:= -pipe -Wno-attributes -fno-math-errno -O3 -g -mtune=pentium4 -msse2 -march=i686 -idirafter /@unixroot/usr/include/os2tk45 -DOS2
LDFLAGS	:= -Zhigh-mem -Zomf -Zargs-wild -Zargs-resp -Zlinker DISABLE -Zlinker 1121
LIBS	:= -lcx -lmmpm2 -lpthread


OBJECTS := $(BUILD_DIR)/$(TARGET).res misc/os2/classicube.def
CC		:= gcc
OEXT	:= .exe
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# Extra file generation
#---------------------------------------------------------------------------------
$(BUILD_DIR)/$(TARGET).res: misc/os2/$(TARGET).rc misc/os2/$(TARGET).ico
	wrc -r misc/os2/$(TARGET).rc -fo=$@


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk
