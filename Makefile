###########################################################
# If target platform isn't specified, default to current OS
###########################################################
ifndef $(PLAT)
	ifeq ($(OS),Windows_NT)
		PLAT = mingw
	else
		PLAT = $(shell uname -s | tr '[:upper:]' '[:lower:]')
	endif
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

# Build for the specified platform
#   "$(filter-out $@, $(MAKECMDGOALS))" is used to get all goals except the current one
# that way, e.g. "make freebsd clean" invokes freebsd makefile with 'clean' goal
web:
	$(MAKE) -f misc/makefiles/Makefile_web.mk     $(filter-out $@, $(MAKECMDGOALS))
linux:
	$(MAKE) -f misc/makefiles/Makefile_linux.mk   $(filter-out $@, $(MAKECMDGOALS))
mingw:
	$(MAKE) -f misc/makefiles/Makefile_windows.mk $(filter-out $@, $(MAKECMDGOALS))
sunos:
	$(MAKE) -f misc/makefiles/Makefile_solaris.mk $(filter-out $@, $(MAKECMDGOALS))
hp-ux:
	$(MAKE) -f misc/makefiles/Makefile_hpux.mk    $(filter-out $@, $(MAKECMDGOALS))
darwin:
	$(MAKE) -f misc/makefiles/Makefile_macos.mk   $(filter-out $@, $(MAKECMDGOALS))
freebsd:
	$(MAKE) -f misc/makefiles/Makefile_freebsd.mk $(filter-out $@, $(MAKECMDGOALS))
openbsd:
	$(MAKE) -f misc/makefiles/Makefile_openbsd.mk $(filter-out $@, $(MAKECMDGOALS))
netbsd:
	$(MAKE) -f misc/makefiles/Makefile_netbsd.mk  $(filter-out $@, $(MAKECMDGOALS))
dragonfly:
	$(MAKE) -f misc/makefiles/Makefile_flybsd.mk  $(filter-out $@, $(MAKECMDGOALS))
haiku:
	$(MAKE) -f misc/makefiles/Makefile_haiku.mk   $(filter-out $@, $(MAKECMDGOALS))
beos:
	$(MAKE) -f misc/makefiles/Makefile_beos.mk    $(filter-out $@, $(MAKECMDGOALS))
serenityos:
	$(MAKE) -f misc/makefiles/Makefile_serenityos.mk $(filter-out $@, $(MAKECMDGOALS)) 
irix:
	$(MAKE) -f misc/makefiles/Makefile_irix.mk    $(filter-out $@, $(MAKECMDGOALS))   
riscos:
	$(MAKE) -f misc/makefiles/Makefile_riscos.mk  $(filter-out $@, $(MAKECMDGOALS))   
gnu:
	$(MAKE) -f misc/makefiles/Makefile_gnuhurd.mk $(filter-out $@, $(MAKECMDGOALS))

# Mobile systems
ios:
	$(MAKE) -f misc/ios/Makefile $(filter-out $@, $(MAKECMDGOALS))
android:
	$(MAKE) -f misc/android/Makefile $(filter-out $@, $(MAKECMDGOALS))

# Embedded systems
wince:
	$(MAKE) -f misc/makefiles/Makefile_wince.mk $(filter-out $@, $(MAKECMDGOALS))
rpi:
	$(MAKE) -f misc/makefiles/Makefile_rpi.mk $(filter-out $@, $(MAKECMDGOALS))

# SEGA consoles
32x:
	$(MAKE) -f misc/32x/Makefile $(filter-out $@, $(MAKECMDGOALS))
saturn:
	$(MAKE) -f misc/saturn/Makefile $(filter-out $@, $(MAKECMDGOALS))
dreamcast:
	$(MAKE) -f misc/dreamcast/Makefile $(filter-out $@, $(MAKECMDGOALS))

# Sony consoles
psp:
	$(MAKE) -f misc/psp/Makefile $(filter-out $@, $(MAKECMDGOALS))
vita:
	$(MAKE) -f misc/vita/Makefile $(filter-out $@, $(MAKECMDGOALS))
ps1:
	$(MAKE) -f misc/ps1/Makefile $(filter-out $@, $(MAKECMDGOALS))
ps2:
	$(MAKE) -f misc/ps2/Makefile $(filter-out $@, $(MAKECMDGOALS))
ps3:
	$(MAKE) -f misc/ps3/Makefile $(filter-out $@, $(MAKECMDGOALS))
ps4:
	$(MAKE) -f misc/ps4/Makefile $(filter-out $@, $(MAKECMDGOALS))

# Microsoft consoles
xbox:
	$(MAKE) -f misc/xbox/Makefile $(filter-out $@, $(MAKECMDGOALS))
xbox360:
	$(MAKE) -f misc/xbox360/Makefile $(filter-out $@, $(MAKECMDGOALS))

# Nintendo consoles
n64:
	$(MAKE) -f misc/n64/Makefile $(filter-out $@, $(MAKECMDGOALS))
gba:
	$(MAKE) -f misc/gba/Makefile $(filter-out $@, $(MAKECMDGOALS))
ds:
	$(MAKE) -f misc/nds/Makefile $(filter-out $@, $(MAKECMDGOALS))
3ds:
	$(MAKE) -f misc/3ds/Makefile $(filter-out $@, $(MAKECMDGOALS))
gamecube:
	$(MAKE) -f misc/gc/Makefile $(filter-out $@, $(MAKECMDGOALS))
wii:
	$(MAKE) -f misc/wii/Makefile $(filter-out $@, $(MAKECMDGOALS))
wiiu:
	$(MAKE) -f misc/wiiu/Makefile $(filter-out $@, $(MAKECMDGOALS))
switch:
	$(MAKE) -f misc/switch/Makefile $(filter-out $@, $(MAKECMDGOALS))

# Other systems
os/2:
	$(MAKE) -f misc/os2/Makefile $(filter-out $@, $(MAKECMDGOALS))
dos:
	$(MAKE) -f misc/msdos/Makefile $(filter-out $@, $(MAKECMDGOALS))
macclassic_68k:
	$(MAKE) -f misc/macclassic/Makefile_68k.mk $(filter-out $@, $(MAKECMDGOALS))
macclassic_ppc:
	$(MAKE) -f misc/macclassic/Makefile_ppc.mk $(filter-out $@, $(MAKECMDGOALS))
amiga_gcc:
	$(MAKE) -f misc/amiga/Makefile_68k $(filter-out $@, $(MAKECMDGOALS))
amiga:
	$(MAKE) -f misc/amiga/Makefile $(filter-out $@, $(MAKECMDGOALS))
atari_st:
	$(MAKE) -f misc/atari_st/Makefile $(filter-out $@, $(MAKECMDGOALS))

# Cleans up all build .o files (except when clean goal is from e.g 'make freebsd clean')
ifeq ($(MAKECMDGOALS),clean)
clean:
	$(MAKE) $(PLAT) clean
else
clean:
	@echo "NOTE: Skipping 'clean' due to not being the only goal (all goals: $(MAKECMDGOALS))"
endif
