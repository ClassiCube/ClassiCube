#ifndef CS_BITMAP_H
#define CS_BITMAP_H
#include "Typedefs.h"
#include "Stream.h"
/* Represents a 2D array of pixels.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

typedef struct Bitmap_ {
	/* Pointer to first scaneline. */
	UInt8* Scan0;
	/* Number of bytes in each scanline. */
	Int32 Stride; /* TODO: Obsolete this completely and just use Width << 2 ? */
	/* Number of pixels horizontally. */
	Int32 Width;
	/* Number of pixels vertically. */
	Int32 Height;
} Bitmap;


/* Size of each ARGB pixel in bytes. */
#define Bitmap_PixelBytesSize 4
/* Calculates size of data of a 2D bitmap in bytes. */
#define Bitmap_DataSize(width, height) ((UInt32)(width) * (UInt32)(height) * (UInt32)Bitmap_PixelBytesSize)
/* Returns a pointer to the start of the y'th scanline. */
#define Bitmap_GetRow(bmp, y) ((UInt32*)((bmp)->Scan0 + ((y) * (bmp)->Stride)))

/* Constructs or updates a Bitmap instance. */
void Bitmap_Create(Bitmap* bmp, Int32 width, Int32 height, UInt8* scan0);
/* Copies a block of pixels from one bitmap to another. */
void Bitmap_CopyBlock(Int32 srcX, Int32 srcY, Int32 dstX, Int32 dstY, Bitmap* src, Bitmap* dst, Int32 size);
/* Copies a row of pixels from one bitmap to another. */
void Bitmap_CopyRow(Int32 srcY, Int32 dstY, Bitmap* src, Bitmap* dst, Int32 width);
/* Allocates a new bitmap of the given dimensions. You are responsible for freeing its memory! */
void Bitmap_Allocate(Bitmap* bmp, Int32 width, Int32 height);
/* Decodes a PNG bitmap from the given stream. */
void Bitmap_DecodePng(Bitmap* bmp, Stream* stream);
#endif