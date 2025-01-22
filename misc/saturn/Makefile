ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Undefined YAUL_INSTALL_ROOT (install root directory))
endif

SH_BUILD_DIR	:= build-saturn

include $(YAUL_INSTALL_ROOT)/share/build.pre.mk

# Each asset follows the format: <path>;<symbol>. Duplicates are removed
BUILTIN_ASSETS=

SH_PROGRAM		:= ClassiCube-saturn
SH_SRCS			:= $(wildcard src/*.c)

SH_CFLAGS+= -Os -I. -DPLAT_SATURN -Wno-sign-compare -Wno-unused-parameter -Wno-missing-field-initializers
SH_LDFLAGS+=

IP_VERSION:= V1.000
IP_RELEASE_DATE:= 20230101
IP_AREAS:= JTUBKAEL
IP_PERIPHERALS:= JAMKST
IP_TITLE:= ClassiCube
#IP_MASTER_STACK_ADDR:= 0x06004000
IP_MASTER_STACK_ADDR:= 0x06100000
IP_SLAVE_STACK_ADDR:= 0x06001E00
IP_1ST_READ_ADDR:= 0x06004000
IP_1ST_READ_SIZE:= 0

include $(YAUL_INSTALL_ROOT)/share/build.post.iso-cue.mk
