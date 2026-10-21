ifeq ($(strip $(VBCC)),)
$(error "VBCC must be set in your environment")
endif
# if $VBCC is not set to root of vbcc folder, vc will fail with 'No config file'


#---------------------------------------------------------------------------------
# Configurable options
#---------------------------------------------------------------------------------
TARGET		:= ClassiCube-atari
SOURCE_DIRS	:= src src/atari_st
BUILD_DIR 	:= build/atari_st
RUN_PROGRAM	?= hatari


#---------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------
CC 		:= vc +tos
CFLAGS 	:= -O1 -DPLAT_ATARIOS

AS 		:= vasmm68k_mot
ASFLAGS := -Fvobj -m68000 -no-fpu

LDFLAGS := -final
LIBS  	:= -lm
LINK	:= $(CC)


#---------------------------------------------------------------------------------
# executable generation
#---------------------------------------------------------------------------------
OEXT := .tos
TRACK_DEPENDENCIES := 0
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk
