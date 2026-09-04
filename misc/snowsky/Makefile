# Build ClassiCube for the Snowsky Echo Disc (FiiO / Ingenic X2000).
#
# The device runs embedded Linux on a MIPS32r2 (little-endian, O32, NaN2008) core, so this
# cross-compiles with zig (https://ziglang.org) acting as the C compiler + MIPS toolchain.
# It reuses the POSIX platform layer and a software rasteriser (no GPU/GL on this device).
#
# Usage (from the repository root):
#     make -f misc/snowsky/Makefile              # SoftGPU (default)
#     make -f misc/snowsky/Makefile GFX=SOFTFP   # fixed-point rasteriser (no FPU reliance)
#
# See misc/snowsky/readme.md for how to deploy and run it on the device.

TARGET  := ClassiCube-snowsky
ZIG     ?= zig
GFX     ?= SOFTGPU

CFLAGS  := -target mipsel-linux-musleabi -static \
           -march=mips32r2 -mabi=32 -mnan=2008 \
           -O2 -fno-strict-aliasing \
           -DPLAT_SNOWSKY -DCC_GFX_BACKEND=CC_GFX_BACKEND_$(GFX)
LIBS    := -lm -lunwind

$(TARGET): src/*.c
	$(ZIG) cc $(CFLAGS) -o $@ src/*.c $(LIBS)
	@# zig omits the NaN2008 ELF flag even with -mnan=2008; the device's kernel needs it.
	python3 misc/snowsky/set_nan2008.py $@

clean:
	rm -f $(TARGET)

.PHONY: clean
