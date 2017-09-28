#include "DisplayDevice.h"

DisplayResolution DisplayResolution_Make(Int32 width, Int32 height, Int32 bitsPerPixel, Real32 refreshRate) {
	DisplayResolution res;
	res.Width = width; res.Height = height;
	res.BitsPerPixel = bitsPerPixel; res.RefreshRate = refreshRate;
	return res;
}

DisplayDevice DisplayDevice_Make(DisplayResolution* curResolution) {
	DisplayDevice device;
	device.CurResolution = *curResolution;
	device.Bounds = Rectangle2D_Make(0, 0, curResolution->Width, curResolution->Height);
	device.Metadata = NULL;
	return device;
}

void DisplayDevice_SetBounds(DisplayDevice* device, Rectangle2D* bounds) {
	device->Bounds = *bounds;
	device->CurResolution.Width = bounds->Width;
	device->CurResolution.Height = bounds->Height;
}

#if !USE_DX

ColorFormat ColorFormat_FromBPP(Int32 bpp) {
	ColorFormat format;
	format.R = 0; format.G = 0; format.B = 0; format.A = 0;
	format.BitsPerPixel = bpp;
	format.IsIndexed = false;
	UInt8 rba;

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
		rba = (UInt8)(bpp / 4);
		format.R = rba; format.B = rba; format.A = rba;
		format.G = (UInt8)((bpp / 4) + (bpp % 4));
		break;
	}
	return format;
}

ColorFormat ColorFormat_FromRGBA(UInt8 r, UInt8 g, UInt8 b, UInt8 a) {
	ColorFormat format;
	format.R = r; format.G = g; format.B = b; format.A = a;
	format.BitsPerPixel = r + g + b + a;
	format.IsIndexed = format.BitsPerPixel < 15 && format.BitsPerPixel != 0;
	return format;
}

GraphicsMode GraphicsMode_Make(ColorFormat color, UInt8 depth, UInt8 stencil, UInt8 buffers) {
	GraphicsMode mode;
	mode.Format = color;
	mode.Depth = depth;
	mode.Stencil = stencil;
	mode.Buffers = buffers;
	return mode;
}

GraphicsMode GraphicsMode_MakeDefault(void) {
	Int32 bpp = DisplayDevice_Default.CurResolution.BitsPerPixel;
	ColorFormat format = ColorFormat_FromBPP(bpp);
	return GraphicsMode_Make(format, 24, 0, 2);
}
#endif