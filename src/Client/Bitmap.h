#ifndef CS_BITMAP_H
#define CS_BITMAP_H
#include "Typedefs.h"
/* Represents a 2D array of pixels.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

typedef struct Bitmap {
	/* Pointer to first scaneline. */
	UInt8* Scan0;

	/* Number of bytes in each scanline. */
	Int32 Stride;

	/* Number of pixels horizontally. */
	Int32 Width;

	/* Number of pixels vertically. */
	Int32 Height;
} Bitmap;


/* Size of each ARGB pixel in bytes. */
#define Bitmap_PixelBytesSize 4

/* Calculates size of data of a 2D bitmap in bytes. */
#define Bitmap_DataSize(width, height) (UInt32)width * (UInt32)height * Bitmap_PixelBytesSize


/* Constructs or updates a Bitmap instance. */
void Bitmap_Create(Bitmap* bmp, Int32 width, Int32 height, Int32 stride, UInt8* scan0);

/* Returns a pointer to the start of the y'th scanline. */
UInt32* Bitmap_GetRow(Bitmap* bmp, Int32 y);

/* Copies a block of pixels from one bitmap to another. */
void Bitmap_CopyBlock(Int32 srcX, Int32 srcY, Int32 dstX, Int32 dstY, Bitmap* src, Bitmap* dst, Int32 size);

/* Copies a row of pixels from one bitmap to another. */
void Bitmap_CopyRow(Int32 srcY, Int32 dstY, Bitmap* src, Bitmap* dst, Int32 width);

/* Allocates a new bitmap of the given dimensions. You are responsible for freeing its memory! */
void Bitmap_Allocate(Bitmap* bmp, Int32 width, Int32 height);
#endif