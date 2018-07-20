#ifndef CC_BITMAP_H
#define CC_BITMAP_H
#include "Typedefs.h"
/* Represents a 2D array of pixels.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct Stream;
struct Bitmap {
	UInt8* Scan0;  /* Pointer to first scaneline. */
	UInt32 Stride; /* Number of bytes in each scanline. TODO: Obsolete this completely and just use Width << 2 ? */	
	Int32 Width;   /* Number of pixels horizontally. */	
	Int32 Height;  /* Number of pixels vertically. */
};

#define PNG_MAX_DIMS 0x8000L
#define BITMAP_SIZEOF_PIXEL 4
#define Bitmap_DataSize(width, height) ((UInt32)(width) * (UInt32)(height) * (UInt32)BITMAP_SIZEOF_PIXEL)
#define Bitmap_GetRow(bmp, y) ((UInt32*)((bmp)->Scan0 + ((y) * (bmp)->Stride)))
#define Bitmap_GetPixel(bmp, x, y) (Bitmap_GetRow(bmp, y)[x])

void Bitmap_Create(struct Bitmap* bmp, Int32 width, Int32 height, UInt8* scan0);
void Bitmap_CopyBlock(Int32 srcX, Int32 srcY, Int32 dstX, Int32 dstY, struct Bitmap* src, struct Bitmap* dst, Int32 size);
/* Allocates a new bitmap of the given dimensions. You are responsible for freeing its memory! */
void Bitmap_Allocate(struct Bitmap* bmp, Int32 width, Int32 height);
/* Allocates a power-of-2 sized bitmap larger or equal to to the given size, and clears it to 0. You are responsible for freeing its memory! */
void Bitmap_AllocateClearedPow2(struct Bitmap* bmp, Int32 width, Int32 height);

bool Bitmap_DetectPng(UInt8* data, UInt32 len);
/*
  Partially based off information from
     https://handmade.network/forums/wip/t/2363-implementing_a_basic_png_reader_the_handmade_way
     https://github.com/nothings/stb/blob/master/stb_image.h
*/
void Bitmap_DecodePng(struct Bitmap* bmp, struct Stream* stream);
void Bitmap_EncodePng(struct Bitmap* bmp, struct Stream* stream);
#endif
