#--------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
#
# NO_ICON: if set to anything, do not use icon.
# NO_NACP: if set to anything, no .nacp file is generated.
# APP_TITLE is the name of the app stored in the .nacp file (Optional)
# APP_AUTHOR is the author of the app stored in the .nacp file (Optional)
# APP_VERSION is the version of the app stored in the .nacp file (Optional)
# APP_TITLEID is the titleID of the app stored in the .nacp file (Optional)
# ICON is the filename of the icon (.jpg), relative to the project folder.
#   If not set, it attempts to use one of the following (in this order):
#     - <Project name>.jpg
#     - icon.jpg
#     - <libnx folder>/default_icon.jpg
#
# CONFIG_JSON is the filename of the NPDM config file (.json), relative to the project folder.
#   If not set, it attempts to use one of the following (in this order):
#     - <Project name>.json
#     - config.json
#   If a JSON file is provided or autodetected, an ExeFS PFS0 (.nsp) is built instead
#   of a homebrew executable (.nro). This is intended to be used for sysmodules.
#   NACP building is skipped as well.
#---------------------------------------------------------------------------------
TARGET		:=	ClassiCube
BUILD_DIR :=	build
SOURCE_DIR:=	src
DATA			:=	data
C_SOURCES   := $(wildcard $(SOURCE_DIR)/*.c)
C_OBJECTS   := $(patsubst $(SOURCE_DIR)/%.c, $(BUILD_DIR)/%.o, $(C_SOURCES))

OBJECTS := $(C_OBJECTS) $(BUILD_DIR)/$(TARGET).res misc/os2/classicube.def
ENAME   = ClassiCube
DEL     = rm -f
APP_TITLE 	:= ClassiCube
APP_AUTHOR 	:= UnknownShadow200

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
CC      := gcc
CFLAGS	:= -pipe -fno-math-errno -O3 -g -mtune=pentium4 -msse2 -march=i686 -idirafter /@unixroot/usr/include/os2tk45 -DOS2
LDFLAGS	:=	-Zhigh-mem -Zomf -Zargs-wild -Zargs-resp -Zlinker DISABLE -Zlinker 1121
LIBS	  := -lcx -lmmpm2 -lpthread -lSDL2

$(TARGET).exe: $(BUILD_DIR) $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(C_OBJECTS): $(BUILD_DIR)/%.o : $(SOURCE_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD_DIR)/$(TARGET).res: misc/os2/$(TARGET).rc misc/os2/$(TARGET).ico
	wrc -r misc/os2/$(TARGET).rc -fo=$@

