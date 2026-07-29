.PHONY: clean run


ifeq ($(OS),Windows_NT)
    HOST := windows
else
    HOST := $(shell uname -s | tr '[:upper:]' '[:lower:]')
endif

# If target platform isn't specified, default to current OS
ifndef $(PLAT)
    PLAT := $(HOST)
endif


default: $(PLAT)

# Shortcuts for default platform
sdl2:
	$(MAKE) $(PLAT) BUILD_SDL2=1
sdl3:
	$(MAKE) $(PLAT) BUILD_SDL3=1
terminal:
	$(MAKE) $(PLAT) BUILD_TERMINAL=1
release:
	$(MAKE) $(PLAT) RELEASE=1


ifeq ($(HOST),linux)
    JOBS := $(shell nproc)
else ifeq ($(HOST),darwin)
    JOBS := $(shell sysctl -n hw.logicalcpu)
endif
# default to 1 job (1 per core)
ifeq ($(strip $(JOBS)),)
    JOBS := 1
endif


# Build for the specified platform
#   "$(filter-out $@, $(MAKECMDGOALS))" is used to get all goals except the current one
# that way, e.g. "make freebsd clean" invokes freebsd makefile with 'clean' goal
define make_platform
	$(MAKE) -f $(1) $(filter-out $@,$(MAKECMDGOALS)) -j $(JOBS)
endef

web:
	$(call make_platform,misc/makefiles/web.mk)
linux:
	$(call make_platform,misc/makefiles/linux.mk)
windows:
	$(call make_platform,misc/makefiles/windows.mk)
sunos:
	$(call make_platform,misc/makefiles/solaris.mk)
hp-ux:
	$(call make_platform,misc/makefiles/hpux.mk)
darwin:
	$(call make_platform,misc/makefiles/macos.mk)
freebsd:
	$(call make_platform,misc/makefiles/freebsd.mk)
openbsd:
	$(call make_platform,misc/makefiles/openbsd.mk)
netbsd:
	$(call make_platform,misc/makefiles/netbsd.mk)
dragonfly:
	$(call make_platform,misc/makefiles/flybsd.mk)
haiku:
	$(call make_platform,misc/makefiles/haiku.mk)
beos:
	$(call make_platform,misc/makefiles/beos.mk)
serenityos:
	$(call make_platform,misc/makefiles/serenityos.mk) 
irix:
	$(call make_platform,misc/makefiles/irix.mk)   
riscos:
	$(call make_platform,misc/makefiles/riscos.mk)   
gnu:
	$(call make_platform,misc/makefiles/gnuhurd.mk)

# Mobile systems
ios:
	$(call make_platform,misc/ios/Makefile)
android:
	$(call make_platform,misc/android/Makefile)

# Embedded systems
wince:
	$(call make_platform,misc/makefiles/wince.mk)
rpi:
	$(call make_platform,misc/makefiles/rpi.mk)

# SEGA consoles
32x:
	$(call make_platform,misc/32x/Makefile)
saturn:
	$(call make_platform,misc/saturn/Makefile)
dreamcast:
	$(call make_platform,misc/dreamcast/Makefile)

# Sony consoles
psp:
	$(call make_platform,misc/psp/Makefile)
vita:
	$(call make_platform,misc/vita/Makefile)
ps1:
	$(call make_platform,misc/ps1/Makefile)
ps2:
	$(call make_platform,misc/ps2/Makefile)
ps3:
	$(call make_platform,misc/ps3/Makefile)
ps4:
	$(call make_platform,misc/ps4/Makefile)

# Microsoft consoles
xbox:
	$(call make_platform,misc/xbox/Makefile)
xbox360:
	$(call make_platform,misc/xbox360/Makefile)

# Nintendo consoles
n64:
	$(call make_platform,misc/n64/Makefile)
gba:
	$(call make_platform,misc/gba/Makefile)
ds:
	$(call make_platform,misc/nds/Makefile)
3ds:
	$(call make_platform,misc/3ds/Makefile)
gamecube:
	$(call make_platform,misc/gc/Makefile)
wii:
	$(call make_platform,misc/wii/Makefile)
wiiu:
	$(call make_platform,misc/wiiu/Makefile)
switch:
	$(call make_platform,misc/switch/Makefile)

# Other systems
os/2:
	$(call make_platform,misc/makefiles/os2.mk)
dos:
	$(call make_platform,misc/makefiles/msdos.mk)
macclassic_68k:
	$(call make_platform,misc/makefiles/macclassic_68k.mk)
macclassic_ppc:
	$(call make_platform,misc/makefiles/macclassic_ppc.mk)
amiga_gcc:
	$(call make_platform,misc/amiga/Makefile_68k)
amiga:
	$(call make_platform,misc/amiga/Makefile)
atari_st:
	$(call make_platform,misc/atari_st/Makefile)


###########################################################
# Global shared/common makefile rules
###########################################################
# Cleans up built files (except when clean goal is from e.g 'make freebsd clean')
ifeq ($(MAKECMDGOALS),clean)
clean:
	$(MAKE) $(PLAT) clean
else
clean:
	@echo "NOTE: Skipping 'clean' due to not being the only goal (all goals: $(MAKECMDGOALS))"
endif

# Compiles for platform and then runs it
ifeq ($(MAKECMDGOALS),run)
run:
	$(MAKE) $(PLAT) run
else
run:
	@echo "NOTE: Skipping 'run' due to not being the only goal (all goals: $(MAKECMDGOALS))"
endif

