BUILD_DIR 	:= build/msdos
SOURCE_DIRS	:= src src/msdos
TARGET 		:= ClassiCube

 # performance too slow if not in release mode
RELEASE		:= 1
include misc/makefiles/common_config.mk
TARGET		:= CCDOS


#---------------------------------------------------------------------------------
# executable generation
#---------------------------------------------------------------------------------
CC		:= i586-pc-msdosdjgpp-gcc
OEXT	:= .EXE
include misc/makefiles/common_build.mk


#---------------------------------------------------------------------------------
# common targets
#---------------------------------------------------------------------------------
include misc/makefiles/common_targets.mk
