#-------------------------------------
# Common configurable flags and names
#-------------------------------------
# Name of the main executable
TARGET := ClassiCube
# Flags passed to the C compiler
CFLAGS += -pipe -fno-math-errno -Werror -Wno-error=missing-braces -Wno-error=strict-aliasing
# Flags passed to the linker
#LDFLAGS +=
# Libraries to link against
#LIBS    +=

# Optimisation level in release builds
OPT_LEVEL ?= 2
# Linker executable (overridable)
LINK ?= $(CC)


#----------------------------------------------------------------
# Platform independent flags
#----------------------------------------------------------------
ifdef BUILD_SDL2
	CFLAGS += -DCC_WIN_BACKEND=CC_WIN_BACKEND_SDL2
	LIBS += -lSDL2
endif
ifdef BUILD_SDL3
	CFLAGS += -DCC_WIN_BACKEND=CC_WIN_BACKEND_SDL3
	LIBS += -lSDL3
endif
ifdef BUILD_TERMINAL
	CFLAGS += -DCC_WIN_BACKEND=CC_WIN_BACKEND_TERMINAL -DCC_GFX_BACKEND=CC_GFX_BACKEND_SOFTGPU
	LDFLAGS := $(subst mwindows,mconsole,$(LDFLAGS))
endif

ifdef RELEASE
	CFLAGS  += -O$(OPT_LEVEL)
else
	CFLAGS  += -g
endif

