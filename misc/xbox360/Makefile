#---------------------------------------------------------------------------------
# Clear the implicit built in rules
#---------------------------------------------------------------------------------
.SUFFIXES:

#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITXENON)),)
$(error "Please set DEVKITXENON in your environment. export DEVKITXENON=<path to>devkitPPC")
endif

#---------------------------------------------------------------------------------
export LIBXENON_INC	:= $(DEVKITXENON)/usr/include
export LIBXENON_LIB	:= $(DEVKITXENON)/usr/lib
LDSCRIPT 			:= $(DEVKITXENON)/app.lds

MACHDEP = -DXENON -m32 -maltivec -fno-pic -mpowerpc64 -mhard-float -L$(DEVKITXENON)/xenon/lib/32

export AS		:= xenon-as
export CC		:= xenon-gcc
export CXX		:= xenon-g++
export AR       := xenon-ar


#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
TARGET		:=	ClassiCube-xbox360
BUILD		:=	build
SOURCES		:=	src

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------

CFLAGS	= -g -O2 -Wall $(MACHDEP) $(INCLUDE)
CXXFLAGS=	$(CFLAGS)

LDFLAGS	=	-g $(MACHDEP) -Wl,--gc-sections -Wl,-Map,$(notdir $@).map

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
LIBS	:=	-lxenon -lm

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
export OUTPUT	:=	$(CURDIR)/$(TARGET)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

#---------------------------------------------------------------------------------
# automatically build a list of object files for our project
#---------------------------------------------------------------------------------
CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
sFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.S)))

export LD		:=	$(CC)
export OFILES	:=	$(CFILES:.c=.o) $(sFILES:.s=.o) $(SFILES:.S=.o)

#---------------------------------------------------------------------------------
# build a list of include paths
#---------------------------------------------------------------------------------
export INCLUDE	:= -I$(LIBXENON_INC)

#---------------------------------------------------------------------------------
# build a list of library paths
#---------------------------------------------------------------------------------
export LIBPATHS	:= -L$(LIBXENON_LIB)

export OUTPUT	:=	$(CURDIR)/$(TARGET)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).elf32: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)

default: $(BUILD) $(OUTPUT).elf32
	cp $(OUTPUT).elf32 /tftpboot/xenon
	xenon-strip /tftpboot/xenon


#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(OUTPUT).elf $(OUTPUT).elf32
	
#---------------------------------------------------------------------------------
%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	@$(CC) -x assembler-with-cpp $(ASFLAGS) -c $< -o $@

%.o: %.S
	@$(CC) -x assembler-with-cpp $(ASFLAGS) -c $< -o $@

%.elf:
	@echo linking ... $(notdir $@)
	@$(LD)  $^ $(LDFLAGS) $(LIBPATHS) $(LIBS) -n -T $(LDSCRIPT) -o $@

%.elf32: %.elf
	@echo converting and stripping ... $(notdir $@)
	@xenon-objcopy -O elf32-powerpc --adjust-vma 0x80000000 $< $@
	@xenon-strip $@
