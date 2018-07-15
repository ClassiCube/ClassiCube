#include "Drawer2D.h"
#include "GraphicsAPI.h"
#include "Funcs.h"
#include "Platform.h"
#include "ExtMath.h"
#include "ErrorHandler.h"
#include "Texture.h"

void DrawTextArgs_Make(struct DrawTextArgs* args, STRING_REF String* text, struct FontDesc* font, bool useShadow) {
	args->Text = *text;
	args->Font = *font;
	args->UseShadow = useShadow;
}

void DrawTextArgs_MakeEmpty(struct DrawTextArgs* args, struct FontDesc* font, bool useShadow) {
	args->Text = String_MakeNull();
	args->Font = *font;
	args->UseShadow = useShadow;
}

struct Bitmap Drawer2D_FontBitmap;
struct Bitmap* Drawer2D_Cur;
Int32 Drawer2D_BoxSize;
/* So really 16 characters per row */
#define DRAWER2D_LOG2_CHARS_PER_ROW 4
Int32 Drawer2D_Widths[256];

static void Drawer2D_CalculateTextWidths(void) {
	Int32 width = Drawer2D_FontBitmap.Width, height = Drawer2D_FontBitmap.Height;
	Int32 i;
	for (i = 0; i < Array_Elems(Drawer2D_Widths); i++) {
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
				UInt8 a = PackedCol_ARGB_A(pixel);
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

static void Drawer2D_FreeFontBitmap(void) {
	Platform_MemFree(&Drawer2D_FontBitmap.Scan0);
}

void Drawer2D_SetFontBitmap(struct Bitmap* bmp) {
	Drawer2D_FreeFontBitmap();
	Drawer2D_FontBitmap = *bmp;
	Drawer2D_BoxSize = bmp->Width >> DRAWER2D_LOG2_CHARS_PER_ROW;
	Drawer2D_CalculateTextWidths();
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
	PackedCol col = PACKEDCOL_CONST(0, 0, 0, 0);
	for (i = 0; i < DRAWER2D_MAX_COLS; i++) {
		Drawer2D_Cols[i] = col;
	}

	for (i = 0; i <= 9; i++) {
		Drawer2D_HexEncodedCol('0' + i, i, 191, 64);
	}
	for (i = 10; i <= 15; i++) {
		Drawer2D_HexEncodedCol('a' + (i - 10), i, 191, 64);
		Drawer2D_HexEncodedCol('a' + (i - 10), i, 191, 64);
	}
}

void Drawer2D_Free(void) {
	Drawer2D_FreeFontBitmap();
}

void Drawer2D_Begin(struct Bitmap* bmp) {
	if (!Drawer2D_UseBitmappedChat) Platform_SetBitmap(bmp);
	Drawer2D_Cur = bmp;
}

void Drawer2D_End(void) {	
	if (!Drawer2D_UseBitmappedChat) Platform_ReleaseBitmap();
	Drawer2D_Cur = NULL;
}

/* Draws a 2D flat rectangle. */
void Drawer2D_Rect(struct Bitmap* bmp, PackedCol col, Int32 x, Int32 y, Int32 width, Int32 height);

void Drawer2D_Clear(struct Bitmap* bmp, PackedCol col, Int32 x, Int32 y, Int32 width, Int32 height) {
	if (x < 0 || y < 0 || (x + width) > bmp->Width || (y + height) > bmp->Height) {
		ErrorHandler_Fail("Drawer2D_Clear - tried to clear at invalid coords");
	}

	Int32 xx, yy;
	UInt32 argb = PackedCol_ToARGB(col);
	for (yy = 0; yy < height; yy++) {
		UInt32* row = Bitmap_GetRow(bmp, y + yy) + x;
		for (xx = 0; xx < width; xx++) { row[xx] = argb; }
	}
}

Int32 Drawer2D_FontHeight(struct FontDesc* font, bool useShadow) {
	struct DrawTextArgs args;
	String text = String_FromConst("I");
	DrawTextArgs_Make(&args, &text, font, useShadow);
	return Drawer2D_MeasureText(&args).Height;
}

void Drawer2D_MakeTextTexture(struct Texture* tex, struct DrawTextArgs* args, Int32 windowX, Int32 windowY) {
	struct Size2D size = Drawer2D_MeasureText(args);
	if (size.Width == 0 && size.Height == 0) {
		Texture_FromOrigin(tex, NULL, windowX, windowY, 0, 0, 1.0f, 1.0f); return;
	}

	struct Bitmap bmp; Bitmap_AllocateClearedPow2(&bmp, size.Width, size.Height);
	Drawer2D_Begin(&bmp);
	{
		Drawer2D_DrawText(args, 0, 0);
	}
	Drawer2D_End();

	Drawer2D_Make2DTexture(tex, &bmp, size, windowX, windowY);
	Platform_MemFree(&bmp.Scan0);
}

void Drawer2D_Make2DTexture(struct Texture* tex, struct Bitmap* bmp, struct Size2D used, Int32 windowX, Int32 windowY) {
	GfxResourceID texId = Gfx_CreateTexture(bmp, false, false);
	Texture_FromOrigin(tex, texId, windowX, windowY, used.Width, used.Height,
		(Real32)used.Width / (Real32)bmp->Width, (Real32)used.Height / (Real32)bmp->Height);
}

bool Drawer2D_ValidColCodeAt(STRING_PURE String* text, Int32 i) {
	if (i >= text->length) return false;
	return Drawer2D_Cols[text->buffer[i]].A > 0;
}
bool Drawer2D_ValidColCode(UChar c) { return Drawer2D_Cols[c].A > 0; }

bool Drawer2D_IsEmptyText(STRING_PURE String* text) {
	if (text->length == 0) return true;

	Int32 i;
	for (i = 0; i < text->length; i++) {
		if (text->buffer[i] != '&') return false;
		if (!Drawer2D_ValidColCodeAt(text, i + 1)) return false;
		i++; /* skip colour code */
	}
	return true;
}

UChar Drawer2D_LastCol(STRING_PURE String* text, Int32 start) {
	if (start >= text->length) start = text->length - 1;
	Int32 i;
	for (i = start; i >= 0; i--) {
		if (text->buffer[i] != '&') continue;
		if (Drawer2D_ValidColCodeAt(text, i + 1)) {
			return text->buffer[i + 1];
		}
	}
	return NULL;
}
bool Drawer2D_IsWhiteCol(UChar c) { return c == NULL || c == 'f' || c == 'F'; }

#define Drawer2D_ShadowOffset(point) (point / 8)
#define Drawer2D_XPadding(point) (Math_CeilDiv(point, 8))
static Int32 Drawer2D_Width(Int32 point, Int32 value) { 
	return Math_CeilDiv(Drawer2D_Widths[value] * point, Drawer2D_BoxSize); 
}
static Int32 Drawer2D_AdjHeight(Int32 point) { return Math_CeilDiv(point * 3, 2); }

void Drawer2D_ReducePadding_Tex(struct Texture* tex, Int32 point, Int32 scale) {
	if (!Drawer2D_UseBitmappedChat) return;

	Int32 padding = (tex->Height - point) / scale;
	Real32 vAdj = (Real32)padding / Math_NextPowOf2(tex->Height);
	tex->V1 += vAdj; tex->V2 -= vAdj;
	tex->Height -= (UInt16)(padding * 2);
}

void Drawer2D_ReducePadding_Height(Int32* height, Int32 point, Int32 scale) {
	if (!Drawer2D_UseBitmappedChat) return;

	Int32 padding = (*height - point) / scale;
	*height -= padding * 2;
}

static void Drawer2D_DrawCore(struct DrawTextArgs* args, Int32 x, Int32 y, bool shadow) {
	PackedCol black = PACKEDCOL_BLACK;
	PackedCol col = Drawer2D_Cols['f'];
	if (shadow) {
		col = Drawer2D_BlackTextShadows ? black : PackedCol_Scale(col, 0.25f);
	}

	String text = args->Text;
	Int32 point = args->Font.Size, count = 0, i;

	UInt8 coords[256];
	PackedCol cols[256];
	UInt16 dstWidths[256];

	for (i = 0; i < text.length; i++) {
		UChar c = text.buffer[i];
		if (c == '&' && Drawer2D_ValidColCodeAt(&text, i + 1)) {
			col = Drawer2D_Cols[text.buffer[i + 1]];
			if (shadow) {
				col = Drawer2D_BlackTextShadows ? black : PackedCol_Scale(col, 0.25f);
			}
			i++; continue; /* skip over the colour code */
		}

		coords[count] = c;
		cols[count] = col;
		dstWidths[count] = Drawer2D_Width(point, c);
		count++;
	}

	Int32 dstHeight = point, startX = x, xx, yy;
	/* adjust coords to make drawn text match GDI fonts */
	Int32 xPadding = Drawer2D_XPadding(point);
	Int32 yPadding = (Drawer2D_AdjHeight(dstHeight) - dstHeight) / 2;

	for (yy = 0; yy < dstHeight; yy++) {
		Int32 dstY = y + (yy + yPadding);
		if (dstY >= Drawer2D_Cur->Height) return;

		Int32 fontY = 0 + yy * Drawer2D_BoxSize / dstHeight;
		UInt32* dstRow = Bitmap_GetRow(Drawer2D_Cur, dstY);

		for (i = 0; i < count; i++) {
			Int32 srcX = (coords[i] & 0x0F) * Drawer2D_BoxSize;
			Int32 srcY = (coords[i] >> 4)   * Drawer2D_BoxSize;
			UInt32* fontRow = Bitmap_GetRow(&Drawer2D_FontBitmap, fontY + srcY);

			Int32 srcWidth = Drawer2D_Widths[coords[i]], dstWidth = dstWidths[i];
			col = cols[i];

			for (xx = 0; xx < dstWidth; xx++) {
				Int32 fontX = srcX + xx * srcWidth / dstWidth;
				UInt32 src = fontRow[fontX];
				if (PackedCol_ARGB_A(src) == 0) continue;

				Int32 dstX = x + xx;
				if (dstX >= Drawer2D_Cur->Width) break;

				UInt32 pixel = src & ~0xFFFFFF;
				pixel |= ((src & 0xFF)         * col.B / 255);
				pixel |= (((src >> 8) & 0xFF)  * col.G / 255) << 8;
				pixel |= (((src >> 16) & 0xFF) * col.R / 255) << 16;
				dstRow[dstX] = pixel;
			}
			x += dstWidth + xPadding;
		}
		x = startX;
	}
}

static void Drawer2D_DrawUnderline(struct DrawTextArgs* args, Int32 x, Int32 y, bool shadow) {
	Int32 point = args->Font.Size, dstHeight = point, startX = x, xx;
	/* adjust coords to make drawn text match GDI fonts */
	Int32 xPadding = Drawer2D_XPadding(point), i;
	Int32 yPadding = (Drawer2D_AdjHeight(dstHeight) - dstHeight) / 2;

	/* scale up bottom row of a cell to drawn text font */
	Int32 startYY = (8 - 1) * dstHeight / 8, yy;
	for (yy = startYY; yy < dstHeight; yy++) {
		Int32 dstY = y + (yy + yPadding);
		if (dstY >= Drawer2D_Cur->Height) return;
		UInt32* dstRow = Bitmap_GetRow(Drawer2D_Cur, dstY);

		PackedCol col = Drawer2D_Cols['f'];
		String text = args->Text;

		for (i = 0; i < text.length; i++) {
			UChar c = text.buffer[i];
			if (c == '&' && Drawer2D_ValidColCodeAt(&text, i + 1)) {
				col = Drawer2D_Cols[text.buffer[i + 1]];
				i++; continue; // Skip over the colour code.
			}

			Int32 dstWidth = Drawer2D_Width(point, c);
			UInt32 argb = shadow ? PackedCol_ARGB(0, 0, 0, 255) : PackedCol_ToARGB(col);

			for (xx = 0; xx < dstWidth + xPadding; xx++) {
				if (x >= Drawer2D_Cur->Width) break;
				dstRow[x++] = argb;
			}
		}
		x = startX;
	}
}

void Drawer2D_DrawBitmapText(struct DrawTextArgs* args, Int32 x, Int32 y) {
	bool ul = args->Font.Style == FONT_STYLE_UNDERLINE;
	Int32 offset = Drawer2D_ShadowOffset(args->Font.Size);

	if (args->UseShadow) {
		Drawer2D_DrawCore(args, x + offset, y + offset, true);
		if (ul) Drawer2D_DrawUnderline(args, x + offset, y + offset, true);
	}

	Drawer2D_DrawCore(args, x, y, false);
	if (ul) Drawer2D_DrawUnderline(args, x, y, false);
}

struct Size2D Drawer2D_MeasureBitmapText(struct DrawTextArgs* args) {
	Int32 point = args->Font.Size;
	/* adjust coords to make drawn text match GDI fonts */
	Int32 xPadding = Drawer2D_XPadding(point), i;
	struct Size2D total = { 0, Drawer2D_AdjHeight(point) };

	String text = args->Text;
	for (i = 0; i < text.length; i++) {
		UChar c = text.buffer[i];
		if (c == '&' && Drawer2D_ValidColCodeAt(&text, i + 1)) {
			i++; continue; /* skip over the colour code */
		}
		total.Width += Drawer2D_Width(point, c) + xPadding;
	}

	/* TODO: this should be uncommented */
	/* Remove padding at end */
	/*if (total.Width > 0) total.Width -= xPadding;*/

	if (args->UseShadow) {
		Int32 offset = Drawer2D_ShadowOffset(point);
		total.Width += offset; total.Height += offset;
	}
	return total;
}

static Int32 Drawer2D_NextPart(Int32 i, STRING_REF String* value, STRING_TRANSIENT String* part, UChar* nextCol) {
	Int32 length = 0, start = i;
	for (; i < value->length; i++) {
		if (value->buffer[i] == '&' && Drawer2D_ValidColCodeAt(value, i + 1)) break;
		length++;
	}

	*part = String_UNSAFE_Substring(value, start, length);
	i += 2; /* skip over colour code */

	if (i <= value->length) *nextCol = value->buffer[i - 1];
	return i;
}

void Drawer2D_DrawText(struct DrawTextArgs* args, Int32 x, Int32 y) {
	if (Drawer2D_IsEmptyText(&args->Text)) return;
	if (Drawer2D_UseBitmappedChat) { Drawer2D_DrawBitmapText(args, x, y); return; }
	
	String value = args->Text;
	UChar nextCol = 'f';
	Int32 i = 0;

	while (i < value.length) {
		UChar colCode = nextCol;
		i = Drawer2D_NextPart(i, &value, &args->Text, &nextCol);
		PackedCol col = Drawer2D_Cols[colCode];
		if (args->Text.length == 0) continue;

		if (args->UseShadow) {
			PackedCol black = PACKEDCOL_BLACK;
			PackedCol backCol = Drawer2D_BlackTextShadows ? black : PackedCol_Scale(col, 0.25f);
			Platform_TextDraw(args, x + DRAWER2D_OFFSET, y + DRAWER2D_OFFSET, backCol);
		}

		struct Size2D partSize = Platform_TextDraw(args, x, y, col);
		x += partSize.Width;
	}
	args->Text = value;
}

struct Size2D Drawer2D_MeasureText(struct DrawTextArgs* args) {
	if (Drawer2D_IsEmptyText(&args->Text)) return Size2D_Empty;
	if (Drawer2D_UseBitmappedChat) return Drawer2D_MeasureBitmapText(args);

	String value = args->Text;
	UChar nextCol = 'f';
	Int32 i = 0;
	struct Size2D size = { 0, 0 };

	while (i < value.length) {
		UChar col = nextCol;
		i = Drawer2D_NextPart(i, &value, &args->Text, &nextCol);
		if (args->Text.length == 0) continue;

		struct Size2D partSize = Platform_TextMeasure(args);
		size.Width += partSize.Width;
		size.Height = max(size.Height, partSize.Height);
	}

	/* TODO: Is this font shadow offet right? */
	if (args->UseShadow) { size.Width += DRAWER2D_OFFSET; size.Height += DRAWER2D_OFFSET; }
	args->Text = value;
	return size;
}
