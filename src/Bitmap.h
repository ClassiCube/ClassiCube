#ifndef CC_BITMAP_H
#define CC_BITMAP_H
#include "Core.h"
/* Represents a 2D array of pixels.
   Copyright 2014-2021 ClassiCube | Licensed under BSD-3
*/
struct Stream;

/* Represents a packed 32 bit RGBA colour, suitable for native graphics API texture pixels. */
typedef cc_uint32 BitmapCol;
#if defined CC_BUILD_WEB || defined CC_BUILD_ANDROID
#define BITMAPCOL_R_SHIFT  0
#define BITMAPCOL_G_SHIFT  8
#define BITMAPCOL_B_SHIFT 16
#define BITMAPCOL_A_SHIFT 24
#else
#define BITMAPCOL_B_SHIFT  0
#define BITMAPCOL_G_SHIFT  8
#define BITMAPCOL_R_SHIFT 16
#define BITMAPCOL_A_SHIFT 24
#endif

#define BITMAPCOL_R_MASK (0xFFU << BITMAPCOL_R_SHIFT)
#define BITMAPCOL_G_MASK (0xFFU << BITMAPCOL_G_SHIFT)
#define BITMAPCOL_B_MASK (0xFFU << BITMAPCOL_B_SHIFT)
#define BITMAPCOL_A_MASK (0xFFU << BITMAPCOL_A_SHIFT)

#define BitmapCol_R(col) ((cc_uint8)(col >> BITMAPCOL_R_SHIFT))
#define BitmapCol_G(col) ((cc_uint8)(col >> BITMAPCOL_G_SHIFT))
#define BitmapCol_B(col) ((cc_uint8)(col >> BITMAPCOL_B_SHIFT))
#define BitmapCol_A(col) ((cc_uint8)(col >> BITMAPCOL_A_SHIFT))

#define BitmapCol_R_Bits(col) ((cc_uint8)(col) << BITMAPCOL_R_SHIFT)
#define BitmapCol_G_Bits(col) ((cc_uint8)(col) << BITMAPCOL_G_SHIFT)
#define BitmapCol_B_Bits(col) ((cc_uint8)(col) << BITMAPCOL_B_SHIFT)
#define BitmapCol_A_Bits(col) ((cc_uint8)(col) << BITMAPCOL_A_SHIFT)

#define BitmapCol_Make(r, g, b, a) (BitmapCol_R_Bits(r) | BitmapCol_G_Bits(g) | BitmapCol_B_Bits(b) | BitmapCol_A_Bits(a))
#define BITMAPCOL_RGB_MASK (BITMAPCOL_R_MASK | BITMAPCOL_G_MASK | BITMAPCOL_B_MASK)

#define BITMAPCOL_BLACK BitmapCol_Make(  0,   0,   0, 255)
#define BITMAPCOL_WHITE BitmapCol_Make(255, 255, 255, 255)

/* A 2D array of BitmapCol pixels */
struct Bitmap { BitmapCol* scan0; int width, height; };
#define PNG_MAX_DIMS 0x8000

/* Returns number of bytes a bitmap consumes. */
#define Bitmap_DataSize(width, height) ((cc_uint32)(width) * (cc_uint32)(height) * 4)
/* Gets the yth row of the given bitmap. */
#define Bitmap_GetRow(bmp, y) ((bmp)->scan0 + (y) * (bmp)->width)
/* Gets the pixel at (x,y) in the given bitmap. */
/* NOTE: Does NOT check coordinates are inside the bitmap. */
#define Bitmap_GetPixel(bmp, x, y) (Bitmap_GetRow(bmp, y)[x])

/* Initialises a bitmap instance. */
#define Bitmap_Init(bmp, width_, height_, scan0_) bmp.width = width_; bmp.height = height_; bmp.scan0 = scan0_;
/* Copies a rectangle of pixels from one bitmap to another. */
/* NOTE: If src and dst are the same, src and dst rectangles MUST NOT overlap. */
/* NOTE: Rectangles are NOT checked for whether they lie inside the bitmaps. */
void Bitmap_UNSAFE_CopyBlock(int srcX, int srcY, int dstX, int dstY, 
							struct Bitmap* src, struct Bitmap* dst, int size);
/* Allocates a new bitmap of the given dimensions. */
/* NOTE: You are responsible for freeing its memory! */
void Bitmap_Allocate(struct Bitmap* bmp, int width, int height);
/* Attemps to allocates a new bitmap of the given dimensions. */
/* NOTE: You are responsible for freeing its memory! */
void Bitmap_TryAllocate(struct Bitmap* bmp, int width, int height);
/* Allocates a power-of-2 sized bitmap equal to or greater than the given size, and clears it to 0. */
/* NOTE: You are responsible for freeing its memory! */
void Bitmap_AllocateClearedPow2(struct Bitmap* bmp, int width, int height);
/* Attempts to allocate a power-of-2 sized bitmap >= than the given size, and clears it to 0. */
/* NOTE: You are responsible for freeing its memory! */
void Bitmap_TryAllocateClearedPow2(struct Bitmap* bmp, int width, int height);
/* Scales a region of the source bitmap to occupy the entirety of the destination bitmap. */
/* The pixels from the region are scaled upwards or downwards depending on destination width and height. */
CC_API void Bitmap_Scale(struct Bitmap* dst, struct Bitmap* src, 
						int srcX, int srcY, int srcWidth, int srcHeight);

/* Whether data starts with PNG format signature/identifier. */
cc_bool Png_Detect(const cc_uint8* data, cc_uint32 len);
typedef int (*Png_RowSelector)(struct Bitmap* bmp, int row);
/*
  Decodes a bitmap in PNG format. Partially based off information from
     https://handmade.network/forums/wip/t/2363-implementing_a_basic_png_reader_the_handmade_way
     https://github.com/nothings/stb/blob/master/stb_image.h
*/
CC_API cc_result Png_Decode(struct Bitmap* bmp, struct Stream* stream);
/* Encodes a bitmap in PNG format. */
/* selectRow is optional. Can be used to modify how rows are encoded. (e.g. flip image) */
/* if alpha is non-zero, RGBA channels are saved, otherwise only RGB channels are. */
CC_API cc_result Png_Encode(struct Bitmap* bmp, struct Stream* stream, 
							Png_RowSelector selectRow, cc_bool alpha);
#endif
