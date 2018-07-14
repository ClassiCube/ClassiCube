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

typedef struct DisplayDevice_ {
	Int32 Width, Height, BitsPerPixel;
	/* The vertical refresh rate of this display. */
	Int32 RefreshRate;
	Rectangle2D Bounds;
	void* Metadata;
} DisplayDevice;
/* The primary / default / main display device. */
DisplayDevice DisplayDevice_Default;

#if !CC_BUILD_D3D9
typedef struct ColorFormat_ {
	UInt8 R, G, B, A;
	bool IsIndexed;
	Int32 BitsPerPixel;
} ColorFormat;

ColorFormat ColorFormat_FromBPP(Int32 bpp);
ColorFormat ColorFormat_FromRGBA(UInt8 r, UInt8 g, UInt8 b, UInt8 a);

typedef struct GraphicsMode_ {
	ColorFormat Format;
	UInt8 DepthBits, StencilBits;
	/* The number of buffers associated with this DisplayMode. */
	UInt8 Buffers;	
} GraphicsMode;

GraphicsMode GraphicsMode_Make(ColorFormat color, UInt8 depth, UInt8 stencil, UInt8 buffers);
/* Returns an GraphicsMode compatible with the underlying platform. */
GraphicsMode GraphicsMode_MakeDefault(void);
#endif
#endif
