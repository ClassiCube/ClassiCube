BUILD_DIR		:= build-dreamcast
SOURCE_DIRS		:= src third_party/bearssl/src

C_FILES := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
OBJS 	:= $(notdir $(C_FILES:%.c=%.o))

TARGET := ClassiCube-dc.elf

ifeq ($(strip $(KOS_BASE)),)
$(error "Please set KOS variables in your environment.")
endif

include $(KOS_BASE)/Makefile.rules

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS)