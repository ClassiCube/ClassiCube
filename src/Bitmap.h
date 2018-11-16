#ifndef CC_BITMAP_H
#define CC_BITMAP_H
#include "Core.h"
/* Represents a 2D array of pixels.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct Stream;

/* Represents an ARGB colour, suitable for native graphics API texture pixels. */
typedef CC_ALIGN_HINT(4) struct BitmapCol_ {
	uint8_t B, G, R, A;
} BitmapCol;
/* Represents an ARGB colour, suitable for native graphics API texture pixels. */
/* Unioned with Packed member for efficient equality comparison */
typedef union BitmapColUnion_ { BitmapCol C; uint32_t Raw; } BitmapColUnion;

typedef struct Bitmap_ { uint8_t* Scan0; int Width, Height; } Bitmap;

#define PNG_MAX_DIMS 0x8000
#define BITMAPCOL_CONST(r, g, b, a) { b, g, r, a }
#define BITMAP_SIZEOF_PIXEL 4 /* 32 bit ARGB */

#define Bitmap_DataSize(width, height) ((uint32_t)(width) * (uint32_t)(height) * (uint32_t)BITMAP_SIZEOF_PIXEL)
#define Bitmap_RawRow(bmp, y) ((uint32_t*)((bmp)->Scan0 + ((y) * ((bmp)->Width << 2))))
#define Bitmap_GetRow(bmp, y) ((BitmapCol*)((bmp)->Scan0 + ((y) * ((bmp)->Width << 2))))
#define Bitmap_GetPixel(bmp, x, y) (Bitmap_GetRow(bmp, y)[x])

BitmapCol BitmapCol_Scale(BitmapCol value, float t);
void Bitmap_Create(Bitmap* bmp, int width, int height, uint8_t* scan0);
void Bitmap_CopyBlock(int srcX, int srcY, int dstX, int dstY, Bitmap* src, Bitmap* dst, int size);
/* Allocates a new bitmap of the given dimensions. You are responsible for freeing its memory! */
void Bitmap_Allocate(Bitmap* bmp, int width, int height);
/* Allocates a power-of-2 sized bitmap larger or equal to to the given size, and clears it to 0. You are responsible for freeing its memory! */
void Bitmap_AllocateClearedPow2(Bitmap* bmp, int width, int height);

bool Png_Detect(const uint8_t* data, uint32_t len);
typedef int (*Png_RowSelector)(Bitmap* bmp, int row);
/*
  Partially based off information from
     https://handmade.network/forums/wip/t/2363-implementing_a_basic_png_reader_the_handmade_way
     https://github.com/nothings/stb/blob/master/stb_image.h
*/
ReturnCode Png_Decode(Bitmap* bmp, struct Stream* stream);
ReturnCode Png_Encode(Bitmap* bmp, struct Stream* stream, Png_RowSelector selectRow);
#endif
