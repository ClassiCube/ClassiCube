ifeq ($(strip $(VBCC)),)
$(error "VBCC must be set in your environment")
endif
# if $VBCC is not set to root of vbcc folder, vc will fail with 'No config file'


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
TARGET		:= ClassiCube-amiga
BUILD_DIR 	:= build/amiga
SOURCE_DIRS	:= src src/amiga


#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------
CC 		:= vc +aos68k
CFLAGS 	:= -rmcfg-amiga-softfloat -O1 -DPLAT_AMIGA

AS 		:= vasmm68k_mot
ASFLAGS := -Fvobj -m68000 -no-fpu

LDFLAGS := -final
LIBS  	:= -lamiga -lmsoft


#---------------------------------------------------------------------------------
# executable generation
#---------------------------------------------------------------------------------
TRACK_DEPENDENCIES := 0
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk
