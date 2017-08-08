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