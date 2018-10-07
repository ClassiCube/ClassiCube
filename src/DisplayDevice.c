#include "DisplayDevice.h"

struct ColorFormat ColorFormat_FromBPP(int bpp) {
	struct ColorFormat format = { 0 };
	format.BitsPerPixel = bpp;
	uint8_t rba;

	switch (bpp) {
	case 32:
		format.R = 8; format.G = 8; format.B = 8; format.A = 8;
		break;
	case 24:
		format.R = 8; format.G = 8; format.B = 8;
		break;
	case 16:
		format.R = 5; format.G = 6; format.B = 5;
		break;
	case 15:
		format.R = 5; format.G = 5; format.B = 5;
		break;
	case 8:
		format.R = 3; format.G = 3; format.B = 2;
		format.IsIndexed = true;
		break;
	case 4:
		format.R = 2; format.G = 2; format.B = 1;
		format.IsIndexed = true;
		break;
	case 1:
		format.IsIndexed = true;
		break;
	default:
		rba = (uint8_t)(bpp / 4);
		format.R = rba; format.B = rba; format.A = rba;
		format.G = (uint8_t)((bpp / 4) + (bpp % 4));
		break;
	}
	return format;
}

struct ColorFormat ColorFormat_FromRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
	struct ColorFormat format;
	format.R = r; format.G = g; format.B = b; format.A = a;
	format.BitsPerPixel = r + g + b + a;
	format.IsIndexed = format.BitsPerPixel < 15 && format.BitsPerPixel != 0;
	return format;
}

struct GraphicsMode GraphicsMode_Make(struct ColorFormat color, uint8_t depth, uint8_t stencil, uint8_t buffers) {
	struct GraphicsMode mode;
	mode.Format = color;
	mode.DepthBits = depth;
	mode.StencilBits = stencil;
	mode.Buffers = buffers;
	return mode;
}

struct GraphicsMode GraphicsMode_MakeDefault(void) {
	int bpp = DisplayDevice_Default.BitsPerPixel;
	struct ColorFormat format = ColorFormat_FromBPP(bpp);
	return GraphicsMode_Make(format, 24, 0, 2);
}
