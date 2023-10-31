SOURCE_DIRS		:= src src/yalloc

C_FILES := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
OBJS 	:= $(notdir $(C_FILES:%.c=%.o))

C_FLAGS = -O3 -DNDEBUG -mfsrra -mfsca -fno-math-errno -ffp-contract=fast -ffast-math -O3 -mpretend-cmove -fexpensive-optimizations -fomit-frame-pointer -finline-functions -ml -m4-single-only -ffunction-sections -fdata-sections -std=gnu99

C_DEFINES = -DDREAMCAST -DNDEBUG -D__DREAMCAST__ -D__arch_dreamcast -D_arch_dreamcast -D_arch_sub_pristine

TARGET := libGLdc.a

ifeq ($(strip $(KOS_BASE)),)
$(error "Please set KOS variables in your environment.")
endif

default: $(TARGET)

%.o: src/%.c
	kos-cc $(C_DEFINES) $(C_FLAGS)  -c $< -o $@

%.o: src/yalloc/%.c
	kos-cc $(C_DEFINES) $(C_FLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	kos-ar cr $@ $^
	kos-ranlib $@