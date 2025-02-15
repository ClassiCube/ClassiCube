#ifndef CC_BITMAP_H
#define CC_BITMAP_H
#include "Core.h"
/* Represents a 2D array of pixels.
   Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/
CC_BEGIN_HEADER

struct Stream;

#if defined CC_BUILD_WEB || defined CC_BUILD_ANDROID || defined CC_BUILD_PSP || defined CC_BUILD_PSVITA || defined CC_BUILD_PS2
	#define BITMAPCOLOR_R_SHIFT  0
	#define BITMAPCOLOR_G_SHIFT  8
	#define BITMAPCOLOR_B_SHIFT 16
	#define BITMAPCOLOR_A_SHIFT 24
#elif defined CC_BUILD_3DS || defined CC_BUILD_N64 || defined CC_BUILD_WIIU
	#define BITMAPCOLOR_R_SHIFT 24
	#define BITMAPCOLOR_G_SHIFT 16
	#define BITMAPCOLOR_B_SHIFT  8
	#define BITMAPCOLOR_A_SHIFT  0
#elif defined BITMAP_16BPP
	#define BITMAPCOLOR_B_SHIFT  0
	#define BITMAPCOLOR_G_SHIFT  5
	#define BITMAPCOLOR_R_SHIFT 10
	#define BITMAPCOLOR_A_SHIFT 15
#elif defined CC_BUILD_PS1 || defined CC_BUILD_SATURN || defined CC_BUILD_NDS || defined CC_BUILD_32X || defined CC_BUILD_GBA
	#define BITMAPCOLOR_R_SHIFT  0
	#define BITMAPCOLOR_G_SHIFT  5
	#define BITMAPCOLOR_B_SHIFT 10
	#define BITMAPCOLOR_A_SHIFT 15
	#define BITMAP_16BPP
#else
	#define BITMAPCOLOR_B_SHIFT  0
	#define BITMAPCOLOR_G_SHIFT  8
	#define BITMAPCOLOR_R_SHIFT 16
	#define BITMAPCOLOR_A_SHIFT 24
#endif

#ifndef BITMAP_16BPP
	/* Represents a packed RGBA colour, suitable for 3D graphics API textures and 2D window framebuffers. */
	typedef cc_uint32 BitmapCol;
	#define BITMAPCOLOR_SIZE 4

	/* Masks a packed color to the selected component */
	#define BITMAPCOLOR_R_MASK (0xFFU << BITMAPCOLOR_R_SHIFT)
	#define BITMAPCOLOR_G_MASK (0xFFU << BITMAPCOLOR_G_SHIFT)
	#define BITMAPCOLOR_B_MASK (0xFFU << BITMAPCOLOR_B_SHIFT)
	#define BITMAPCOLOR_A_MASK (0xFFU << BITMAPCOLOR_A_SHIFT)
	
	/* Extracts just the R/G/B/A component from a bitmap color */
	#define BitmapCol_R(color) ((cc_uint8)((color) >> BITMAPCOLOR_R_SHIFT))
	#define BitmapCol_G(color) ((cc_uint8)((color) >> BITMAPCOLOR_G_SHIFT))
	#define BitmapCol_B(color) ((cc_uint8)((color) >> BITMAPCOLOR_B_SHIFT))
	#define BitmapCol_A(color) ((cc_uint8)((color) >> BITMAPCOLOR_A_SHIFT))
	
	/* Converts input value into a packed color component */
	#define BitmapColor_R_Bits(value) ((cc_uint8)(value) << BITMAPCOLOR_R_SHIFT)
	#define BitmapColor_G_Bits(value) ((cc_uint8)(value) << BITMAPCOLOR_G_SHIFT)
	#define BitmapColor_B_Bits(value) ((cc_uint8)(value) << BITMAPCOLOR_B_SHIFT)
	#define BitmapColor_A_Bits(value) ((cc_uint8)(value) << BITMAPCOLOR_A_SHIFT)
#else
	/* Represents a packed RGBA colour, suitable for 3D graphics API textures and 2D window framebuffers. */
	typedef cc_uint16 BitmapCol;
	#define BITMAPCOLOR_SIZE 2

	/* Masks a packed color to the selected component */
	#define BITMAPCOLOR_R_MASK (0x1FU << BITMAPCOLOR_R_SHIFT)
	#define BITMAPCOLOR_G_MASK (0x1FU << BITMAPCOLOR_G_SHIFT)
	#define BITMAPCOLOR_B_MASK (0x1FU << BITMAPCOLOR_B_SHIFT)
	#define BITMAPCOLOR_A_MASK (0x01U << BITMAPCOLOR_A_SHIFT)
	
	/* Extracts just the R/G/B/A component from a bitmap color */
	#define BitmapCol_R(color) (((cc_uint8)(color >> BITMAPCOLOR_R_SHIFT) & 0x1F) << 3)
	#define BitmapCol_G(color) (((cc_uint8)(color >> BITMAPCOLOR_G_SHIFT) & 0x1F) << 3)
	#define BitmapCol_B(color) (((cc_uint8)(color >> BITMAPCOLOR_B_SHIFT) & 0x1F) << 3)
	#define BitmapCol_A(color) (((cc_uint8)(color >> BITMAPCOLOR_A_SHIFT) & 0x01) << 7)
	
	/* Converts input value into a packed color component */
	#define BitmapColor_R_Bits(value) (((cc_uint8)(value) >> 3) << BITMAPCOLOR_R_SHIFT)
	#define BitmapColor_G_Bits(value) (((cc_uint8)(value) >> 3) << BITMAPCOLOR_G_SHIFT)
	#define BitmapColor_B_Bits(value) (((cc_uint8)(value) >> 3) << BITMAPCOLOR_B_SHIFT)
	#define BitmapColor_A_Bits(value) (((cc_uint8)(value) >> 7) << BITMAPCOLOR_A_SHIFT)
#endif

#define BitmapCol_Make(r, g, b, a) (BitmapColor_R_Bits(r) | BitmapColor_G_Bits(g) | BitmapColor_B_Bits(b) | BitmapColor_A_Bits(a))
#define BitmapColor_RGB(r, g, b)   (BitmapColor_R_Bits(r) | BitmapColor_G_Bits(g) | BitmapColor_B_Bits(b) | BITMAPCOLOR_A_MASK)
#define BITMAPCOLOR_RGB_MASK (BITMAPCOLOR_R_MASK | BITMAPCOLOR_G_MASK | BITMAPCOLOR_B_MASK)

#define BITMAPCOLOR_BLACK BitmapColor_RGB(  0,   0,   0)
#define BITMAPCOLOR_WHITE BitmapColor_RGB(255, 255, 255)

BitmapCol BitmapColor_Offset(BitmapCol color, int rBy, int gBy, int bBy);
BitmapCol BitmapColor_Scale(BitmapCol a, float t);

/* A 2D array of BitmapCol pixels */
struct Bitmap { BitmapCol* scan0; int width, height; };
/* Returns number of bytes a bitmap consumes. */
#define Bitmap_DataSize(width, height) ((cc_uint32)(width) * (cc_uint32)(height) * BITMAPCOLOR_SIZE)
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
/* Scales a region of the source bitmap to occupy the entirety of the destination bitmap. */
/* The pixels from the region are scaled upwards or downwards depending on destination width and height. */
CC_API void Bitmap_Scale(struct Bitmap* dst, struct Bitmap* src, 
						int srcX, int srcY, int srcWidth, int srcHeight);

#define PNG_SIG_SIZE 8
#if defined CC_BUILD_LOWMEM
/* No point supporting > 1K x 1K bitmaps when system has less than 64 MB of RAM anyways */
#define PNG_MAX_DIMS 1024
#else
#define PNG_MAX_DIMS 0x8000
#endif

/* Whether data starts with PNG format signature/identifier. */
cc_bool Png_Detect(const cc_uint8* data, cc_uint32 len);
typedef BitmapCol* (*Png_RowGetter)(struct Bitmap* bmp, int row, void* ctx);
/*
  Decodes a bitmap in PNG format. Partially based off information from
     https://handmade.network/forums/wip/t/2363-implementing_a_basic_png_reader_the_handmade_way
     https://github.com/nothings/stb/blob/master/stb_image.h
*/
CC_API cc_result Png_Decode(struct Bitmap* bmp, struct Stream* stream);
/* Encodes a bitmap in PNG format. */
/* getRow is optional. Can be used to modify how rows are encoded. (e.g. flip image) */
/* if alpha is non-zero, RGBA channels are saved, otherwise only RGB channels are. */
cc_result Png_Encode(struct Bitmap* bmp, struct Stream* stream, 
						Png_RowGetter getRow, cc_bool alpha, void* ctx);

CC_END_HEADER
#endif
