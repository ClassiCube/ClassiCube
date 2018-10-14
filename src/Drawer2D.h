#ifndef CC_DRAWER2D_H
#define CC_DRAWER2D_H
#include "PackedCol.h"
#include "Constants.h"
/*  Responsible for performing drawing operations on bitmaps, and for converting bitmaps into textures.
	Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

struct DrawTextArgs { String Text; FontDesc Font; bool UseShadow; };
struct Texture;

void DrawTextArgs_Make(struct DrawTextArgs* args, STRING_REF const String* text, const FontDesc* font, bool useShadow);
void DrawTextArgs_MakeEmpty(struct DrawTextArgs* args, const FontDesc* font, bool useShadow);
NOINLINE_ void Drawer2D_MakeFont(FontDesc* desc, int size, int style);

/* Whether chat text should be drawn and measuring using the currently bitmapped font, 
 false uses the font supplied as the DrawTextArgs argument supplied to the function. */
bool Drawer2D_BitmappedText;
/* Whether the shadows behind text (that uses shadows) is fully black. */
bool Drawer2D_BlackTextShadows;
PackedCol Drawer2D_Cols[DRAWER2D_MAX_COLS];
#define DRAWER2D_OFFSET 1
#define Drawer2D_GetCol(c) Drawer2D_Cols[(uint8_t)c]

void Drawer2D_Init(void);
void Drawer2D_Free(void);

/* Draws a 2D flat rectangle. */
void Drawer2D_Rect(Bitmap* bmp, PackedCol col, int x, int y, int width, int height);
/* Clears the entire given area to the specified colour. */
void Drawer2D_Clear(Bitmap* bmp, PackedCol col, int x, int y, int width, int height);

void Drawer2D_Underline(Bitmap* bmp, int x, int y, int width, int height, PackedCol col);
void Drawer2D_DrawText(Bitmap* bmp, struct DrawTextArgs* args, int x, int y);
Size2D Drawer2D_MeasureText(struct DrawTextArgs* args);
int Drawer2D_FontHeight(const FontDesc* font, bool useShadow);

void Drawer2D_MakeTextTexture(struct Texture* tex, struct DrawTextArgs* args, int X, int Y);
void Drawer2D_Make2DTexture(struct Texture* tex, Bitmap* bmp, Size2D used, int X, int Y);

bool Drawer2D_ValidColCodeAt(const String* text, int i);
bool Drawer2D_ValidColCode(char c);
bool Drawer2D_IsEmptyText(const String* text);
/* Returns the last valid colour code in the given input, or \0 if no valid colour code was found. */
char Drawer2D_LastCol(const String* text, int start);
bool Drawer2D_IsWhiteCol(char c);

void Drawer2D_ReducePadding_Tex(struct Texture* tex, int point, int scale);
void Drawer2D_ReducePadding_Height(int* height, int point, int scale);
void Drawer2D_SetFontBitmap(Bitmap* bmp);
#endif
