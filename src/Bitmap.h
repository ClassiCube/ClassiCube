#ifndef CC_BITMAP_H
#define CC_BITMAP_H
#include "Core.h"
/* Represents a 2D array of pixels.
   Copyright 2014-2019 ClassiCube | Licensed under BSD-3
*/
struct Stream;

/* Represents an ARGB colour, suitable for native graphics API texture pixels. */
typedef CC_ALIGN_HINT(4) struct BitmapCol_ {
#if defined CC_BUILD_WEB || defined CC_BUILD_ANDROID
	uint8_t R, G, B, A;
#else
	uint8_t B, G, R, A;
#endif
} BitmapCol;
/* Represents an ARGB colour, suitable for native graphics API texture pixels. */
/* Unioned with Packed member for efficient equality comparison */
typedef union BitmapColUnion_ { BitmapCol C; uint32_t Raw; } BitmapColUnion;

/* Scales RGB components of the given colour. */
CC_API BitmapCol BitmapCol_Scale(BitmapCol value, float t);

/* A 2D array of BitmapCol pixels */
typedef struct Bitmap_ { uint8_t* Scan0; int Width, Height; } Bitmap;

#define PNG_MAX_DIMS 0x8000
#if defined CC_BUILD_WEB || defined CC_BUILD_ANDROID
#define BITMAPCOL_CONST(r, g, b, a) { r, g, b, a }
#else
#define BITMAPCOL_CONST(r, g, b, a) { b, g, r, a }
#endif

/* Returns number of bytes a bitmap consumes. */
#define Bitmap_DataSize(width, height) ((uint32_t)(width) * (uint32_t)(height) * 4)
/* Gets the yth row of a bitmap as raw uint32_t* pointer. */
/* NOTE: You SHOULD not rely on the order of the 4 bytes in the pointer. */
/* Different platforms may have different endian, or different component order. */
#define Bitmap_RawRow(bmp, y) ((uint32_t*)(bmp)->Scan0  + (y) * (bmp)->Width)
/* Gets the yth row of the given bitmap. */
#define Bitmap_GetRow(bmp, y) ((BitmapCol*)(bmp)->Scan0 + (y) * (bmp)->Width)
/* Gets the pixel at (x,y) in the given bitmap. */
/* NOTE: Does NOT check coordinates are inside the bitmap. */
#define Bitmap_GetPixel(bmp, x, y) (Bitmap_GetRow(bmp, y)[x])

/* Initialises a bitmap instance. */
#define Bitmap_Init(bmp, width, height, scan0) bmp.Width = width; bmp.Height = height; bmp.Scan0 = scan0;
/* Copies a rectangle of pixels from one bitmap to another. */
/* NOTE: If src and dst are the same, src and dst rectangles MUST NOT overlap. */
/* NOTE: Rectangles are NOT checked for whether they lie inside the bitmaps. */
void Bitmap_CopyBlock(int srcX, int srcY, int dstX, int dstY, Bitmap* src, Bitmap* dst, int size);
/* Allocates a new bitmap of the given dimensions. */
/* NOTE: You are responsible for freeing its memory! */
void Bitmap_Allocate(Bitmap* bmp, int width, int height);
/* Allocates a power-of-2 sized bitmap equal to or greater than the given size, and clears it to 0. */
/* NOTE: You are responsible for freeing its memory! */
void Bitmap_AllocateClearedPow2(Bitmap* bmp, int width, int height);

/* Whether data starts with PNG format signature/identifier. */
bool Png_Detect(const uint8_t* data, uint32_t len);
typedef int (*Png_RowSelector)(Bitmap* bmp, int row);
/*
  Decodes a bitmap in PNG format. Partially based off information from
     https://handmade.network/forums/wip/t/2363-implementing_a_basic_png_reader_the_handmade_way
     https://github.com/nothings/stb/blob/master/stb_image.h
*/
CC_API ReturnCode Png_Decode(Bitmap* bmp, struct Stream* stream);
/* Encodes a bitmap in PNG format. */
/* selectRow is optional. Can be used to modify how rows are encoded. (e.g. flip image) */
/* if alpha is non-zero, RGBA channels are saved, otherwise only RGB channels are. */
CC_API ReturnCode Png_Encode(Bitmap* bmp, struct Stream* stream, Png_RowSelector selectRow, bool alpha);
#endif
