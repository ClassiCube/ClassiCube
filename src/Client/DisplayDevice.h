#ifndef CS_DISPLAYDEVICE_H
#define CS_DISPLAYDEVICE_H
#include "Typedefs.h"
#include "2DStructs.h"
/* Contains structs related to monitor displays.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3 | Based originally on OpenTK
*/

/* Licensed under the MIT/X11 license.
* Copyright (c) 2006-2008 the OpenTK team.
* This notice may not be removed.
* See license.txt for licensing detailed licensing details.
*/

/* Contains information regarding a monitor's display resolution. */
typedef struct DisplayResolution_ {
	/* The width of this display in pixels. */
	Int32 Width;
	/* The height of this display in pixels. */
	Int32 Height;
	/* The number of bits per pixel of this display. Typical values include 8, 16, 24 and 32. */
	Int32 BitsPerPixel;
	/* The vertical refresh rate of this display. */
	Real32 RefreshRate;
} DisplayResolution;

/* Constructs a new display resolution instance. */
DisplayResolution DisplayResolution_Make(Int32 width, Int32 height, Int32 bitsPerPixel, Real32 refreshRate);


/* Defines a display device on the underlying system.*/
typedef struct DisplayDevice_ {
	/* The current resolution of the display device.*/
	DisplayResolution CurResolution;
	/* The bounds of the display device.*/
	Rectangle2D Bounds;
	/* Metadata unique to this display device instance. */
	void* Metadata;
} DisplayDevice;

/* Constructs a new display device instance. */
DisplayDevice DisplayDevice_Make(DisplayResolution* curResolution);
/* Updates the bounds of the display device to the given bounds. */
void DisplayDevice_SetBounds(DisplayDevice* device, Rectangle2D* bounds);
/* The primary / default / main display device. */
DisplayDevice DisplayDevice_Default;
#endif