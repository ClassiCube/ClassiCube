#ifndef CC_SYSTEMFONTS_H
#define CC_SYSTEMFONTS_H
#include "Core.h"
/*  Manages loading and drawing platform specific system fonts
	Copyright 2014-2023 ClassiCube | Licensed under BSD-3
*/
struct Bitmap;
struct FontDesc;
struct DrawTextArgs;
struct IGameComponent;
struct StringsBuffer;
extern struct IGameComponent SystemFonts_Component;

/* Allocates a new system font from the given arguments */
cc_result SysFont_Make(struct FontDesc* desc, const cc_string* fontName, int size, int flags);
/* Frees an allocated system font */
void SysFont_Free(struct FontDesc* desc);
/* Allocates a new system font from the given arguments using default system font */
/*  NOTE: Unlike SysFont_Make, this may fallback onto other system fonts (e.g. Arial, Roboto, etc) */
void SysFont_MakeDefault(struct FontDesc* desc, int size, int flags);
/* Sets default system font name and raises ChatEvents.FontChanged */
void SysFont_SetDefault(const cc_string* fontName);

/* Measures width of the given text when drawn with the given system font */
int SysFont_TextWidth(struct DrawTextArgs* args);
/* Draws the given text with the given system font onto the given bitmap */
void SysFont_DrawText(struct DrawTextArgs* args, struct Bitmap* bmp, int x, int y, cc_bool shadow);

/* Gets the name of the default system font used */
const cc_string* SysFonts_UNSAFE_GetDefault(void);
/* Gets the list of all supported system font names on this platform */
CC_API void SysFonts_GetNames(struct StringsBuffer* buffer);
/* Attempts to decode one or more fonts from the given file */
/*  NOTE: If this file has been decoded before (fontscache.txt), does nothing */
void SysFonts_Register(const cc_string* path);
#endif
