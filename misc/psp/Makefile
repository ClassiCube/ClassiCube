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

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
