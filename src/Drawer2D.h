#ifndef CC_DRAWER2D_H
#define CC_DRAWER2D_H
#include "PackedCol.h"
#include "Constants.h"
#include "Bitmap.h"
/*  Performs a variety of drawing operations on bitmaps, and converts bitmaps into textures.
	Copyright 2014-2021 ClassiCube | Licensed under BSD-3
*/

enum FONT_FLAGS { FONT_FLAGS_NONE = 0x00, FONT_FLAGS_BOLD = 0x01, FONT_FLAGS_UNDERLINE = 0x02, FONT_FLAGS_PADDING = 0x04 };
struct FontDesc { void* handle; cc_uint16 size, flags; int height; };
struct DrawTextArgs { cc_string text; struct FontDesc* font; cc_bool useShadow; };
struct Texture;
struct IGameComponent;
struct StringsBuffer;
extern struct IGameComponent Drawer2D_Component;

CC_VAR extern struct _Drawer2DData {
	/* Whether text should be drawn and measured using the currently set font bitmap. */
	/* If false, then text is instead draw using platform/system fonts. */
	cc_bool BitmappedText;
	/* Whether the shadows behind text (that uses shadows) is fully black. */
	cc_bool BlackTextShadows;
	/* List of all colours. (An A of 0 means the colour is not used) */
	BitmapCol Colors[DRAWER2D_MAX_COLS];
} Drawer2D;

#define Drawer2D_GetCol(c) Drawer2D.Colors[(cc_uint8)c]
void DrawTextArgs_Make(struct DrawTextArgs* args, STRING_REF const cc_string* text, struct FontDesc* font, cc_bool useShadow);
void DrawTextArgs_MakeEmpty(struct DrawTextArgs* args, struct FontDesc* font, cc_bool useShadow);

/* Sets default system font name and raises ChatEvents.FontChanged */
void Drawer2D_SetDefaultFont(const cc_string* fontName);
/* Initialises the given font for drawing bitmapped text using default.png */
void Drawer2D_MakeBitmappedFont(struct FontDesc* desc, int size, int flags);
/* Initialises the given font. Uses Drawer2D_MakeBitmappedFont or Font_MakeDefault depending on Drawer2D_BitmappedText. */
CC_API void Drawer2D_MakeFont(struct FontDesc* desc, int size, int flags);

/* Clamps the given rectangle to lie inside the bitmap. */
/* Returns false if rectangle is completely outside bitmap's rectangle. */
cc_bool Drawer2D_Clamp(struct Bitmap* bmp, int* x, int* y, int* width, int* height);

/* Fills the given area with a simple noisy pattern. */
/* Variation determines how 'visible/obvious' the noise is. */
CC_API void Gradient_Noise(struct Bitmap* bmp, BitmapCol col, int variation,
						   int x, int y, int width, int height);
/* Fills the given area with a vertical gradient, linerarly interpolating from a to b. */
/* First drawn row is filled with 'a', while last drawn is filled with 'b'. */
CC_API void Gradient_Vertical(struct Bitmap* bmp, BitmapCol a, BitmapCol b,
							  int x, int y, int width, int height);
/* Blends the given area with the given colour. */
/* Note that this only blends RGB, A is not blended. */
CC_API void Gradient_Blend(struct Bitmap* bmp, BitmapCol col, int blend,
						   int x, int y, int width, int height);
/* Tints the given area, linearly interpolating from a to b. */
/* Note that this only tints RGB, A is not tinted. */
CC_API void Gradient_Tint(struct Bitmap* bmp, cc_uint8 tintA, cc_uint8 tintB,
						  int x, int y, int width, int height);

/* Fills the given area using pixels from the source bitmap. */
CC_API void Drawer2D_BmpCopy(struct Bitmap* dst, int x, int y, struct Bitmap* src);
/* Fills the area with the given colour. */
CC_API void Drawer2D_Clear(struct Bitmap* bmp, BitmapCol col, 
							int x, int y, int width, int height);

/* Draws text using the given font at the given coordinates. */
CC_API void Drawer2D_DrawText(struct Bitmap* bmp, struct DrawTextArgs* args, int x, int y);
/* Returns how wide the given text would be when drawn. */
CC_API int Drawer2D_TextWidth(struct DrawTextArgs* args);
/* Returns how tall the given text would be when drawn. */
/* NOTE: Height returned only depends on the font. (see Drawer2D_FontHeight). */
CC_API int Drawer2D_TextHeight(struct DrawTextArgs* args);
/* Similar to Drawer2D_DrawText, but trims the text with trailing ".." if wider than maxWidth. */
void Drawer2D_DrawClippedText(struct Bitmap* bmp, struct DrawTextArgs* args, 
								int x, int y, int maxWidth);
/* Returns the line height for drawing any character in the font. */
int Drawer2D_FontHeight(const struct FontDesc* font, cc_bool useShadow);

/* Creates a texture consisting only of the given text drawn onto it. */
/* NOTE: The returned texture is always padded up to nearest power of two dimensions. */
CC_API void Drawer2D_MakeTextTexture(struct Texture* tex, struct DrawTextArgs* args);
/* Creates a texture consisting of the pixels from the given bitmap. */
/* NOTE: bmp must always have power of two dimensions. */
/* width/height specifies what region of the texture actually should be drawn. */
CC_API void Drawer2D_MakeTexture(struct Texture* tex, struct Bitmap* bmp, int width, int height);

/* Returns whether the given colour code is used/valid. */
/* NOTE: This can change if the server defines custom colour codes. */
cc_bool Drawer2D_ValidColCodeAt(const cc_string* text, int i);
/* Whether text is empty or consists purely of valid colour codes. */
cc_bool Drawer2D_IsEmptyText(const cc_string* text);
/* Copies all characters from str into src, except for used colour codes */
/* NOTE: Ampersands not followed by a used colour code are still copied */
void Drawer2D_WithoutCols(cc_string* str, const cc_string* src);
/* Returns the last valid colour code in the given input, or \0 if not found. */
char Drawer2D_LastCol(const cc_string* text, int start);
/* Returns whether the colour code is f, F or \0. */
cc_bool Drawer2D_IsWhiteCol(char c);

void Drawer2D_ReducePadding_Tex(struct Texture* tex, int point, int scale);
void Drawer2D_ReducePadding_Height(int* height, int point, int scale);
/* Sets the bitmap used for drawing bitmapped fonts. (i.e. default.png) */
/* The bitmap must be square and consist of a 16x16 tile layout. */
cc_bool Drawer2D_SetFontBitmap(struct Bitmap* bmp);

/* Gets the name of the default system font used. */
const cc_string* Font_UNSAFE_GetDefault(void);
/* Gets the list of all supported system font names on this platform. */
CC_API void Font_GetNames(struct StringsBuffer* buffer);

/* Sets padding for a bitmapped font. */
void Font_SetPadding(struct FontDesc* desc, int amount);
/* Finds the path and face number of the given system font, with closest matching style */
cc_string Font_Lookup(const cc_string* fontName, int flags);
/* Allocates a new system font from the given arguments. */
cc_result Font_Make(struct FontDesc* desc, const cc_string* fontName, int size, int flags);
/* Allocates a new system font from the given arguments using default system font. */
/* NOTE: Unlike Font_Make, this may fallback onto other system fonts (e.g. Arial, Roboto, etc) */
void Font_MakeDefault(struct FontDesc* desc, int size, int flags);
/* Frees an allocated font. */
CC_API void Font_Free(struct FontDesc* desc);
/* Attempts to decode one or fonts from the given file. */
/* NOTE: If this file has been decoded before (fontscache.txt), does nothing. */
void SysFonts_Register(const cc_string* path);
#endif
