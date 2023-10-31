ifeq ($(strip $(NXDK_DIR)),)
$(error "Please set NXDK_DIR in your environment")
endif

XBE_TITLE 	= ClassiCube
GEN_XISO 	= ClassiCube-xbox.iso
SRCS 		= $(wildcard src/*.c) $(wildcard third_party/bearssl/src/*.c)
SHADER_OBJS = misc/xbox/vs_coloured.inl misc/xbox/vs_textured.inl misc/xbox/ps_coloured.inl misc/xbox/ps_textured.inl
NXDK_NET 	= y
NXDK_CFLAGS = -Ithird_party/bearssl/inc -O1
NXDK_LDFLAGS = -stack:131072

include $(NXDK_DIR)/Makefile