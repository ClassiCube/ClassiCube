#ifndef CC_DRAWER2D_H
#define CC_DRAWER2D_H
#include "PackedCol.h"
#include "Constants.h"
#include "Bitmap.h"
/*  Responsible for performing drawing operations on bitmaps, and for converting bitmaps into textures.
	Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

struct DrawTextArgs { String Text; FontDesc Font; bool UseShadow; };
struct Texture;
struct IGameComponent;
extern struct IGameComponent Drawer2D_Component;

void DrawTextArgs_Make(struct DrawTextArgs* args, STRING_REF const String* text, const FontDesc* font, bool useShadow);
void DrawTextArgs_MakeEmpty(struct DrawTextArgs* args, const FontDesc* font, bool useShadow);
CC_NOINLINE void Drawer2D_MakeFont(FontDesc* desc, int size, int style);

/* Whether chat text should be drawn and measuring using the currently bitmapped font, 
 false uses the font supplied as the DrawTextArgs argument supplied to the function. */
extern bool Drawer2D_BitmappedText;
/* Whether the shadows behind text (that uses shadows) is fully black. */
extern bool Drawer2D_BlackTextShadows;
/* List of all colours. (An A of 0 means the colour is not used) */
extern BitmapCol Drawer2D_Cols[DRAWER2D_MAX_COLS];
#define DRAWER2D_OFFSET 1
#define Drawer2D_GetCol(c) Drawer2D_Cols[(uint8_t)c]

/* Name of default font, defaults to Font_DefaultName. */
extern String Drawer2D_FontName;

/* Clamps the given rectangle to line inside the bitmap. */
/* Returns false if rectangle is completely outside bitmap's rectangle. */
bool Drawer2D_Clamp(Bitmap* bmp, int* x, int* y, int* width, int* height);

void Gradient_Noise(Bitmap* bmp, BitmapCol col, int variation,
					int x, int y, int width, int height);
void Gradient_Vertical(Bitmap* bmp, BitmapCol a, BitmapCol b,
					   int x, int y, int width, int height);
void Gradient_Blend(Bitmap* bmp, BitmapCol col, int blend,
					int x, int y, int width, int height);

void Drawer2D_BmpIndexed(Bitmap* bmp, int x, int y, int size,
						 uint8_t* indices, BitmapCol* palette);
void Drawer2D_BmpScaled(Bitmap* dst, int x, int y, int width, int height,
						Bitmap* src, int srcX, int srcY, int srcWidth, int srcHeight,
						int scaleWidth, int scaleHeight, uint8_t scaleA, uint8_t scaleB);
void Drawer2D_BmpTiled(Bitmap* dst, int x, int y, int width, int height,
					   Bitmap* src, int srcX, int srcY, int srcWidth, int srcHeight);
void Drawer2D_BmpCopy(Bitmap* dst, int x, int y, int width, int height, Bitmap* src);


/* Draws a 2D flat rectangle. */
void Drawer2D_Rect(Bitmap* bmp, BitmapCol col, int x, int y, int width, int height);
/* Fills the given rectangular area with the given colour. */
void Drawer2D_Clear(Bitmap* bmp, BitmapCol col, int x, int y, int width, int height);

void Drawer2D_Underline(Bitmap* bmp, int x, int y, int width, int height, BitmapCol col);
void Drawer2D_DrawText(Bitmap* bmp, struct DrawTextArgs* args, int x, int y);
Size2D Drawer2D_MeasureText(struct DrawTextArgs* args);
int Drawer2D_FontHeight(const FontDesc* font, bool useShadow);

void Drawer2D_MakeTextTexture(struct Texture* tex, struct DrawTextArgs* args, int X, int Y);
void Drawer2D_Make2DTexture(struct Texture* tex, Bitmap* bmp, Size2D used, int X, int Y);

bool Drawer2D_ValidColCodeAt(const String* text, int i);
bool Drawer2D_ValidColCode(char c);
/* Whether text is empty or consists purely of valid colour codes. */
bool Drawer2D_IsEmptyText(const String* text);
/* Returns the last valid colour code in the given input, or \0 if not found. */
char Drawer2D_LastCol(const String* text, int start);
bool Drawer2D_IsWhiteCol(char c);

void Drawer2D_ReducePadding_Tex(struct Texture* tex, int point, int scale);
void Drawer2D_ReducePadding_Height(int* height, int point, int scale);
void Drawer2D_SetFontBitmap(Bitmap* bmp);
#endif
