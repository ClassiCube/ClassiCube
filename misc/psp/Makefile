TARGET 	= ClassiCube-psp
SOURCES	= $(wildcard src/*.c) $(wildcard third_party/bearssl/src/*.c)
OBJS   	= $(patsubst %.c, %.o, $(SOURCES))

INCDIR   = third_party/bearssl/inc
CFLAGS   = -g -O1 -fno-math-errno
ASFLAGS  = $(CFLAGS)

LIBDIR  =
LDFLAGS =
LIBS    = -lm -lpspgum -lpspgu -lpspge -lpspdisplay -lpspctrl

BUILD_PRX = 1

EXTRA_TARGETS   = EBOOT.PBP
PSP_EBOOT_TITLE = ClassiCube
PSP_EBOOT_ICON	= misc/psp/ICON0.png

ifeq ($(strip $(PSPSDK)),)
$(warning "Please set PSPSDK variables in your environment. For example:")
$(warning   export PSPSDK=/usr/local/pspsk/psp/sdk)
$(warning   export PATH=/usr/local/pspsk/bin:$$PATH)
$(warning Or)
$(warning   export PSPSDK=$$(shell psp-config --pspsdk-path))
$(error   Failed to find PSPSDK installation)
endif

include $(PSPSDK)/lib/build.mak
