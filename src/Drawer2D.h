#ifndef CC_DRAWER2D_H
#define CC_DRAWER2D_H
#include "Bitmap.h"
#include "Constants.h"
/*  Performs a variety of drawing operations on bitmaps, and converts bitmaps into textures.
	Copyright 2014-2022 ClassiCube | Licensed under BSD-3
*/

enum FONT_FLAGS { FONT_FLAGS_NONE = 0x00, FONT_FLAGS_BOLD = 0x01, FONT_FLAGS_UNDERLINE = 0x02, FONT_FLAGS_PADDING = 0x04 };
struct FontDesc { void* handle; cc_uint16 size, flags; int height; };
struct DrawTextArgs { cc_string text; struct FontDesc* font; cc_bool useShadow; };
struct Context2D { struct Bitmap bmp; int width, height; void* meta; };
struct Texture;
struct IGameComponent;
struct StringsBuffer;
extern struct IGameComponent Drawer2D_Component;

#define DRAWER2D_MAX_TEXT_LENGTH 256

CC_VAR extern struct _Drawer2DData {
	/* Whether text should be drawn and measured using the currently set font bitmap */
	/* If false, then text is instead draw using platform/system fonts */
	cc_bool BitmappedText;
	/* Whether the shadows behind text (that uses shadows) is fully black */
	cc_bool BlackTextShadows;
	/* List of all colors (An A of 0 means the color is not used) */
	BitmapCol Colors[DRAWER2D_MAX_COLORS];
} Drawer2D;

#define Drawer2D_GetColor(c) Drawer2D.Colors[(cc_uint8)c]
void DrawTextArgs_Make(struct DrawTextArgs* args, STRING_REF const cc_string* text, struct FontDesc* font, cc_bool useShadow);
void DrawTextArgs_MakeEmpty(struct DrawTextArgs* args, struct FontDesc* font, cc_bool useShadow);

/* Clamps the given rectangle to lie inside the bitmap */
/* Returns false if rectangle is completely outside bitmap's rectangle */
cc_bool Drawer2D_Clamp(struct Context2D* ctx, int* x, int* y, int* width, int* height);

/* Allocates a new context for 2D drawing */
/*  Note: Allocates a power-of-2 sized backing bitmap equal to or greater than the given size */
CC_API void Context2D_Alloc(struct Context2D* ctx, int width, int height);
/* Allocates a new context for 2D drawing, using an existing bimap as backing bitmap */
CC_API void Context2D_Wrap(struct Context2D* ctx, struct Bitmap* bmp);
/* Frees/Releases a previously allocatedcontext for 2D drawing */
CC_API void Context2D_Free(struct Context2D* ctx);
/* Creates a texture consisting of the pixels from the backing bitmap of the given 2D context */
CC_API void Context2D_MakeTexture(struct Texture* tex, struct Context2D* ctx);

/* Draws text using the given font at the given coordinates */
CC_API void Context2D_DrawText(struct Context2D* ctx, struct DrawTextArgs* args, int x, int y);
/* Fills the given area using pixels from the source bitmap */
CC_API void Context2D_DrawPixels(struct Context2D* ctx, int x, int y, struct Bitmap* src);
/* Fills the area with the given color */
CC_API void Context2D_Clear(struct Context2D* ctx, BitmapCol color,
							int x, int y, int width, int height);

/* Fills the given area with a simple noisy pattern */
/* Variation determines how 'visible/obvious' the noise is */
CC_API void Gradient_Noise(struct Context2D* ctx, BitmapCol color, int variation,
						   int x, int y, int width, int height);
/* Fills the given area with a vertical gradient, linerarly interpolating from a to b */
/* First drawn row is filled with 'a', while last drawn is filled with 'b' */
CC_API void Gradient_Vertical(struct Context2D* ctx, BitmapCol a, BitmapCol b,
							  int x, int y, int width, int height);
/* Blends the given area with the given color */
/*  Note that this only blends RGB, A is not blended */
CC_API void Gradient_Blend(struct Context2D* ctx, BitmapCol color, int blend,
						   int x, int y, int width, int height);

/* Returns how wide the given text would be when drawn */
CC_API int Drawer2D_TextWidth(struct DrawTextArgs* args);
/* Returns how tall the given text would be when drawn */
/*  NOTE: Height returned only depends on the font. (see Font_CalcHeight) */
CC_API int Drawer2D_TextHeight(struct DrawTextArgs* args);
/* Similar to Context2D_DrawText, but trims the text with trailing ".." if wider than maxWidth */
void Drawer2D_DrawClippedText(struct Context2D* ctx, struct DrawTextArgs* args, 
								int x, int y, int maxWidth);

/* Creates a texture consisting only of the given text drawn onto it */
/*  NOTE: The returned texture is always padded up to nearest power of two dimensions */
CC_API void Drawer2D_MakeTextTexture(struct Texture* tex, struct DrawTextArgs* args);

/* Returns whether the given color code is used/valid */
/* NOTE: This can change if the server defines custom color codes */
cc_bool Drawer2D_ValidColorCodeAt(const cc_string* text, int i);
/* Whether text is empty or consists purely of valid color codes */
cc_bool Drawer2D_IsEmptyText(const cc_string* text);
/* Copies all characters from str into src, except for used color codes */
/*  NOTE: Ampersands not followed by a used color code are still copied */
void Drawer2D_WithoutColors(cc_string* str, const cc_string* src);
/* Returns the last valid color code in the given input, or \0 if not found */
char Drawer2D_LastColor(const cc_string* text, int start);
/* Returns whether the color code is f, F or \0 */
cc_bool Drawer2D_IsWhiteColor(char c);
cc_bool Drawer2D_UNSAFE_NextPart(cc_string* left, cc_string* part, char* colorCode);

/* Allocates a new instance of the default font using the given size and flags */
/*  Uses Font_MakeBitmapped or SysFont_MakeDefault depending on Drawer2D_BitmappedText */
CC_API void Font_Make(struct FontDesc* desc, int size, int flags);
/* Frees an allocated font */
CC_API void Font_Free(struct FontDesc* desc);
/* Sets default system font name and raises ChatEvents.FontChanged */
void Font_SetDefault(const cc_string* fontName);
/* Returns the line height for drawing text using the given font */
int Font_CalcHeight(const struct FontDesc* font, cc_bool useShadow);

void Drawer2D_ReducePadding_Tex(struct Texture* tex, int point, int scale);
void Drawer2D_ReducePadding_Height(int* height, int point, int scale);
/* Sets the bitmap used for drawing bitmapped fonts. (i.e. default.png) */
/* The bitmap must be square and consist of a 16x16 tile layout */
cc_bool Font_SetBitmapAtlas(struct Bitmap* bmp);
/* Sets padding for a bitmapped font */
void Font_SetPadding(struct FontDesc* desc, int amount);
/* Initialises the given font for drawing bitmapped text using default.png */
void Font_MakeBitmapped(struct FontDesc* desc, int size, int flags);

/* Allocates a new system font from the given arguments */
cc_result SysFont_Make(struct FontDesc* desc, const cc_string* fontName, int size, int flags);
/* Allocates a new system font from the given arguments using default system font */
/*  NOTE: Unlike SysFont_Make, this may fallback onto other system fonts (e.g. Arial, Roboto, etc) */
void SysFont_MakeDefault(struct FontDesc* desc, int size, int flags);

/* Gets the name of the default system font used */
const cc_string* SysFonts_UNSAFE_GetDefault(void);
/* Gets the list of all supported system font names on this platform */
CC_API void SysFonts_GetNames(struct StringsBuffer* buffer);
/* Attempts to decode one or more fonts from the given file */
/*  NOTE: If this file has been decoded before (fontscache.txt), does nothing */
void SysFonts_Register(const cc_string* path);
#endif
