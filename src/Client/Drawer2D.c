#include "Drawer2D.h"
#include "GraphicsAPI.h"
#include "Funcs.h"
#include "Platform.h"
#include "ExtMath.h"

void DrawTextArgs_Make(DrawTextArgs* args, STRING_REF String* text, void* font, bool useShadow) {
	args->Text = *text;
	args->Font = font;
	args->UseShadow = useShadow;
}

Bitmap Drawer2D_FontBitmap;
Bitmap* Drawer2D_Cur;
Int32 Drawer2D_BoxSize;
#define DRAWER2D_ITALIC_SIZE 8
/* So really 16 characters per row */
#define DRAWER2D_LOG2_CHARS_PER_ROW 4
Int32 Drawer2D_Widths[256];

void Drawer2D_FreeFontBitmap(void) {
	if (Drawer2D_FontBitmap.Scan0 == NULL) return;
	Platform_MemFree(Drawer2D_FontBitmap.Scan0);
	Drawer2D_FontBitmap.Scan0 = NULL;
}

void Drawer2D_SetFontBitmap(Bitmap bmp) {
	Drawer2D_FreeFontBitmap();
	Drawer2D_FontBitmap = bmp;
	Drawer2D_BoxSize = bmp.Width >> DRAWER2D_LOG2_CHARS_PER_ROW;
	Drawer2D_CalculateTextWidths();
}

void Drawer2D_CalculateTextWidths(void) {
	Int32 width = Drawer2D_FontBitmap.Width, height = Drawer2D_FontBitmap.Height;
	Int32 i;
	for (i = 0; i < Array_NumElements(Drawer2D_Widths); i++) {
		Drawer2D_Widths[i] = 0;
	}

	Int32 x, y, xx;
	for (y = 0; y < height; y++) {
		Int32 charY = (y / Drawer2D_BoxSize);
		UInt32* row = Bitmap_GetRow(&Drawer2D_FontBitmap, y);

		for (x = 0; x < width; x += Drawer2D_BoxSize) {
			Int32 charX = (x / Drawer2D_BoxSize);
			/* Iterate through each pixel of the given character, on the current scanline */
			for (xx = Drawer2D_BoxSize - 1; xx >= 0; xx--) {
				UInt32 pixel = row[x + xx];
				UInt8 a = (UInt8)(pixel >> 24);
				if (a < 127) continue;

				/* Check if this is the pixel furthest to the right, for the current character */
				Int32 index = charX | (charY << DRAWER2D_LOG2_CHARS_PER_ROW);
				Drawer2D_Widths[index] = max(Drawer2D_Widths[index], xx + 1);
				break;
			}
		}
	}
	Drawer2D_Widths[' '] = Drawer2D_BoxSize / 4;
}


void Drawer2D_HexEncodedCol(Int32 i, Int32 hex, UInt8 lo, UInt8 hi) {
	PackedCol* col = &Drawer2D_Cols[i];
	col->R = (UInt8)(lo * ((hex >> 2) & 1) + hi * (hex >> 3));
	col->G = (UInt8)(lo * ((hex >> 1) & 1) + hi * (hex >> 3));
	col->B = (UInt8)(lo * ((hex >> 0) & 1) + hi * (hex >> 3));
	col->A = UInt8_MaxValue;
}

void Drawer2D_Init(void) {
	UInt32 i;
	PackedCol col = PackedCol_Create4(0, 0, 0, 0);
	for (i = 0; i < DRAWER2D_MAX_COLS; i++) {
		Drawer2D_Cols[i] = col;
	}

	for (i = 0; i <= 9; i++) {
		Drawer2D_GetHexEncodedCol('0' + i, i, 191, 64);
	}
	for (i = 10; i <= 15; i++) {
		Drawer2D_GetHexEncodedCol('a' + (i - 10), i, 191, 64);
		Drawer2D_GetHexEncodedCol('a' + (i - 10), i, 191, 64);
	}
}

void Drawer2D_Free(void) {
	Drawer2D_FreeFontBitmap();
}

/* Sets the underlying bitmap that drawing operations are performed on. */
void Drawer2D_Begin(Bitmap* bmp);
/* Frees any resources associated with the underlying bitmap. */
void Drawer2D_End(void);
/* Draws a 2D flat rectangle. */
void Drawer2D_Rect(PackedCol col, Int32 x, Int32 y, Int32 width, Int32 height);

void Drawer2D_Clear(PackedCol col, Int32 x, Int32 y, Int32 width, Int32 height) {
	if (x < 0 || y < 0 || (x + width) > Drawer2D_Cur->Width || (y + height) > Drawer2D_Cur->Height) {
		ErrorHandler_Fail("Drawer2D_Clear - tried to clear at invalid coords");
	}

	Int32 xx, yy;
	UInt32 argb = PackedCol_ToARGB(col);
	for (yy = 0; yy < height; yy++) {
		UInt32* row = Bitmap_GetRow(Drawer2D_Cur, y + yy) + x;
		for (xx = 0; xx < width; xx++) { row[xx] = argb; }
	}
}

void Drawer2D_DrawText(DrawTextArgs* args, Int32 x, Int32 y);
Size2D Drawer2D_MeasureText(DrawTextArgs* args);

Int32 Drawer2D_FontHeight(void* font, bool useShadow) {
	DrawTextArgs args;
	String text = String_FromConstant("I");
	DrawTextArgs_Make(&args, &text, font, useShadow);
	return Drawer2D_MeasureText(&args).Height;
}

Texture Drawer2D_MakeTextTexture(DrawTextArgs* args, Int32 windowX, Int32 windowY) {
	Size2D size = Drawer2D_MeasureText(args);
	if (Size2D_Equals(size, Size2D_Empty)) {
		return Texture_FromOrigin(-1, windowX, windowY, 0, 0, 1.0f, 1.0f);
	}

	Bitmap bmp; Bitmap_AllocatePow2(&bmp, size.Width, size.Height);
	Drawer2D_Begin(&bmp);
	Drawer2D_DrawText(&args, 0, 0);
	Drawer2D_End();
	return Drawer2D_Make2DTexture(&bmp, size, windowX, windowY);
}

Texture Drawer2D_Make2DTexture(Bitmap* bmp, Size2D used, Int32 windowX, Int32 windowY) {
	GfxResourceID texId = Gfx_CreateTexture(bmp, false, false);
	return Texture_FromOrigin(texId, windowX, windowY, used.Width, used.Height,
		(Real32)used.Width / (Real32)bmp->Width, (Real32)used.Height / (Real32)bmp->Height);
}

bool Drawer2D_ValidColCodeAt(STRING_TRANSIENT String* text, Int32 i) {
	if (i >= text->length) return false;
	return Drawer2D_Cols[text->buffer[i]].A > 0;
}
bool Drawer2D_ValidColCode(UInt8 c) { return Drawer2D_Cols[c].A > 0; }

bool Drawer2D_IsEmptyText(STRING_TRANSIENT String* text) {
	if (text->length == 0) return true;

	Int32 i;
	for (i = 0; i < text->length; i++) {
		if (text->buffer[i] != '&') return false;
		if (!ValidColCode(text, i + 1)) return false;
		i++; /* skip colour code */
	}
	return true;
}

UInt8 Drawer2D_LastCol(STRING_TRANSIENT String* text, Int32 start) {
	if (start >= text->length) start = text->length - 1;
	Int32 i;
	for (i = start; i >= 0; i--) {
		if (text->buffer[i] != '&') continue;
		if (Drawer2D_ValidColCode(text, i + 1)) {
			return text->buffer[i + 1];
		}
	}
	return NULL;
}
bool Drawer2D_IsWhiteCol(UInt8 c) { return c == NULL || c == 'f' || c == 'F'; }

Int32 Drawer2D_ShadowOffset(Int32 fontSize) { return fontSize / 8; }
Int32 Drawer2D_Width(Int32 point, Int32 value) { return Math_CeilDiv(value * point, Drawer2D_BoxSize); }
Int32 Drawer2D_PaddedWidth(Int32 point, Int32 value) {
	return Math_CeilDiv(value * point, Drawer2D_BoxSize) + Math_CeilDiv(point, 8);
}
/* Rounds font size up to the nearest whole multiple of the size of a character in default.png. */
Int32 Drawer2D_AdjTextSize(Int32 point) { return point; } /* Math_CeilDiv(point, Drawer2D_BoxSize) * Drawer2D_BoxSize; */
/* Returns the height of the bounding box that contains the font size, in addition to padding. */
Int32 Drawer2D_CellSize(Int32 point) { return Math_CeilDiv(point * 3, 2); }

void Drawer2D_ReducePadding_Tex(Texture* tex, Int32 point, Int32 scale) {
	if (!Drawer2D_UseBitmappedChat) return;
	point = Drawer2D_AdjTextSize(point);

	Int32 padding = (tex->Height - point) / scale;
	Real32 vAdj = (Real32)padding / Math_NextPowerOf2(tex->Height);
	tex->V1 += vAdj; tex->V2 -= vAdj;
	tex->Height -= (UInt16)(padding * 2);
}

void Drawer2D_ReducePadding_Height(Int32* height, Int32 point, Int32 scale) {
	if (!Drawer2D_UseBitmappedChat) return;
	point = Drawer2D_AdjTextSize(point);

	Int32 padding = (*height - point) / scale;
	*height -= padding * 2;
}