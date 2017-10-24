#ifndef CC_DISPLAYDEVICE_H
#define CC_DISPLAYDEVICE_H
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

#if !USE_DX
/* Contains Red, Green, Blue and Alpha components for a color format of the given bits per pixel. */
typedef struct ColorFormat_ {
	/* Gets the bits per pixel for the Red channel. */
	UInt8 R;
	/* Gets the bits per pixel for the Green channel. */
	UInt8 G;
	/* Gets the bits per pixel for the Blue channel. */
	UInt8 B;
	/* Gets the bits per pixel for the Alpha channel. */
	UInt8 A;
	/* Gets whether this ColorFormat is indexed.  */
	bool IsIndexed;
	/* Gets the sum of Red, Green, Blue and Alpha bits per pixel. */
	Int32 BitsPerPixel;
} ColorFormat;

/* Constructs a color format from given bits per pixel. */
ColorFormat ColorFormat_FromBPP(Int32 bpp);
/* Constructs a color format from given bits per component. */
ColorFormat ColorFormat_FromRGBA(UInt8 r, UInt8 g, UInt8 b, UInt8 a);


/* Defines the format for graphics operations. */
typedef struct GraphicsMode_ {
	/* The OpenTK.Graphics.ColorFormat that describes the color format for this GraphicsFormat.  */
	ColorFormat Format;
	/* The bits per pixel for the depth buffer for this GraphicsFormat.  */
	UInt8 Depth;
	/* The bits per pixel for the stencil buffer of this GraphicsFormat.  */
	UInt8 Stencil;
	/* The number of buffers associated with this DisplayMode.  */
	UInt8 Buffers;	
} GraphicsMode;

/* Constructs a new GraphicsMode with the specified parameters. */
GraphicsMode GraphicsMode_Make(ColorFormat color, UInt8 depth, UInt8 stencil, UInt8 buffers);
/* Returns an GraphicsMode compatible with the underlying platform. */
GraphicsMode GraphicsMode_MakeDefault(void);
#endif
#endif