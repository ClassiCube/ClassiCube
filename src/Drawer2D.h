#ifndef CC_DRAWER2D_H
#define CC_DRAWER2D_H
#include "PackedCol.h"
#include "Constants.h"
/*  Responsible for performing drawing operations on bitmaps, and for converting bitmaps into textures.
	Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

struct DrawTextArgs { String Text; FontDesc Font; bool UseShadow; };
struct Texture;

void DrawTextArgs_Make(struct DrawTextArgs* args, STRING_REF String* text, FontDesc* font, bool useShadow);
void DrawTextArgs_MakeEmpty(struct DrawTextArgs* args, FontDesc* font, bool useShadow);

/* Whether chat text should be drawn and measuring using the currently bitmapped font, 
 false uses the font supplied as the DrawTextArgs argument supplied to the function. */
bool Drawer2D_UseBitmappedChat;
/* Whether the shadows behind text (that uses shadows) is fully black. */
bool Drawer2D_BlackTextShadows;
PackedCol Drawer2D_Cols[DRAWER2D_MAX_COLS];
#define DRAWER2D_OFFSET 1

void Drawer2D_Init(void);
void Drawer2D_Free(void);

/* Sets the underlying bitmap that text operations are performed on. */
void Drawer2D_Begin(Bitmap* bmp);
/* Frees any resources associated with the underlying bitmap. */
void Drawer2D_End(void);
/* Draws a 2D flat rectangle. */
void Drawer2D_Rect(Bitmap* bmp, PackedCol col, Int32 x, Int32 y, Int32 width, Int32 height);
/* Clears the entire given area to the specified colour. */
void Drawer2D_Clear(Bitmap* bmp, PackedCol col, Int32 x, Int32 y, Int32 width, Int32 height);

void Drawer2D_DrawText(struct DrawTextArgs* args, Int32 x, Int32 y);
Size2D Drawer2D_MeasureText(struct DrawTextArgs* args);
Int32 Drawer2D_FontHeight(FontDesc* font, bool useShadow);

void Drawer2D_MakeTextTexture(struct Texture* tex, struct DrawTextArgs* args, Int32 X, Int32 Y);
void Drawer2D_Make2DTexture(struct Texture* tex, Bitmap* bmp, Size2D used, Int32 X, Int32 Y);

bool Drawer2D_ValidColCodeAt(STRING_PURE String* text, Int32 i);
bool Drawer2D_ValidColCode(char c);
bool Drawer2D_IsEmptyText(STRING_PURE String* text);
/* Returns the last valid colour code in the given input, or \0 if no valid colour code was found. */
char Drawer2D_LastCol(STRING_PURE String* text, Int32 start);
bool Drawer2D_IsWhiteCol(char c);

void Drawer2D_ReducePadding_Tex(struct Texture* tex, Int32 point, Int32 scale);
void Drawer2D_ReducePadding_Height(Int32* height, Int32 point, Int32 scale);
void Drawer2D_SetFontBitmap(Bitmap* bmp);
#endif
