#ifndef CC_DISPLAYDEVICE_H
#define CC_DISPLAYDEVICE_H
#include "Core.h"
/* Contains structs related to monitor displays.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3 | Based originally on OpenTK
*/

/* Licensed under the MIT/X11 license.
* Copyright (c) 2006-2008 the OpenTK team.
* This notice may not be removed.
* See license.txt for licensing detailed licensing details.
*/

struct DisplayDevice { int BitsPerPixel; Rect2D Bounds; };
struct DisplayDevice DisplayDevice_Default;
void* DisplayDevice_Meta[3];

struct ColorFormat {
	uint8_t R, G, B, A;
	bool IsIndexed;
	int  BitsPerPixel;
};

struct ColorFormat ColorFormat_FromBPP(int bpp);
struct ColorFormat ColorFormat_FromRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

struct GraphicsMode {
	struct ColorFormat Format;
	uint8_t DepthBits, StencilBits;
	/* The number of buffers associated with this DisplayMode. */
	uint8_t Buffers;
};

struct GraphicsMode GraphicsMode_Make(struct ColorFormat color, uint8_t depth, uint8_t stencil, uint8_t buffers);
/* Returns an GraphicsMode compatible with the underlying platform. */
struct GraphicsMode GraphicsMode_MakeDefault(void);
#endif
