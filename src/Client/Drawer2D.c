#include "Drawer2D.h"
#include "GraphicsAPI.h"
#include "Funcs.h"
#include "Platform.h"
#include "ExtMath.h"
#include "ErrorHandler.h"

void DrawTextArgs_Make(DrawTextArgs* args, STRING_REF String* text, FontDesc* font, bool useShadow) {
	args->Text = *text;
	args->Font = *font;
	args->UseShadow = useShadow;
}

void DrawTextArgs_MakeEmpty(DrawTextArgs* args, FontDesc* font, bool useShadow) {
	args->Text = String_MakeNull();
	args->Font = *font;
	args->UseShadow = useShadow;
}

Bitmap Drawer2D_FontBitmap;
Bitmap* Drawer2D_Cur;
Int32 Drawer2D_BoxSize;
/* So really 16 characters per row */
#define DRAWER2D_LOG2_CHARS_PER_ROW 4
Int32 Drawer2D_Widths[256];

void Drawer2D_CalculateTextWidths(void) {
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

void Drawer2D_Begin(Bitmap* bmp) {
	Platform_SetBitmap(bmp);
	Drawer2D_Cur = bmp;
}

void Drawer2D_End(void) {	
	Platform_SetBitmap(Drawer2D_Cur);
	Drawer2D_Cur = NULL;
}

/* Draws a 2D flat rectangle. */
void Drawer2D_Rect(Bitmap* bmp, PackedCol col, Int32 x, Int32 y, Int32 width, Int32 height);

void Drawer2D_Clear(Bitmap* bmp, PackedCol col, Int32 x, Int32 y, Int32 width, Int32 height) {
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

Int32 Drawer2D_FontHeight(FontDesc* font, bool useShadow) {
	DrawTextArgs args;
	String text = String_FromConst("I");
	DrawTextArgs_Make(&args, &text, font, useShadow);
	return Drawer2D_MeasureText(&args).Height;
}

Texture Drawer2D_MakeTextTexture(DrawTextArgs* args, Int32 windowX, Int32 windowY) {
	Size2D size = Drawer2D_MeasureText(args);
	if (size.Width == 0.0f && size.Height == 0.0f) {
		return Texture_FromOrigin(NULL, windowX, windowY, 0, 0, 1.0f, 1.0f);
	}

	Bitmap bmp; Bitmap_AllocatePow2(&bmp, size.Width, size.Height);
	Drawer2D_Begin(&bmp);
	Drawer2D_DrawText(args, 0, 0);
	Drawer2D_End();
	return Drawer2D_Make2DTexture(&bmp, size, windowX, windowY);
}

Texture Drawer2D_Make2DTexture(Bitmap* bmp, Size2D used, Int32 windowX, Int32 windowY) {
	GfxResourceID texId = Gfx_CreateTexture(bmp, false, false);
	return Texture_FromOrigin(texId, windowX, windowY, used.Width, used.Height,
		(Real32)used.Width / (Real32)bmp->Width, (Real32)used.Height / (Real32)bmp->Height);
}

bool Drawer2D_ValidColCodeAt(STRING_PURE String* text, Int32 i) {
	if (i >= text->length) return false;
	return Drawer2D_Cols[text->buffer[i]].A > 0;
}
bool Drawer2D_ValidColCode(UInt8 c) { return Drawer2D_Cols[c].A > 0; }

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

UInt8 Drawer2D_LastCol(STRING_PURE String* text, Int32 start) {
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
	Real32 vAdj = (Real32)padding / Math_NextPowOf2(tex->Height);
	tex->V1 += vAdj; tex->V2 -= vAdj;
	tex->Height -= (UInt16)(padding * 2);
}

void Drawer2D_ReducePadding_Height(Int32* height, Int32 point, Int32 scale) {
	if (!Drawer2D_UseBitmappedChat) return;
	point = Drawer2D_AdjTextSize(point);

	Int32 padding = (*height - point) / scale;
	*height -= padding * 2;
}

void Drawer2D_DrawRun(Int32 x, Int32 y, Int32 runCount, UInt8* coords, Int32 point, PackedCol col) {
	if (runCount == 0) return;
	Int32 srcY = (coords[0] >> 4) * Drawer2D_BoxSize;
	Int32 textHeight = Drawer2D_AdjTextSize(point), cellHeight = Drawer2D_CellSize(textHeight);
	/* inlined xPadding so we don't need to call PaddedWidth */
	Int32 xPadding = Math_CeilDiv(point, 8), yPadding = (cellHeight - textHeight) / 2;
	Int32 startX = x;

	UInt16 dstWidths[1280];
	Int32 i, xx, yy;
	for (i = 0; i < runCount; i++) {
		dstWidths[i] = (UInt16)Drawer2D_Width(point, Drawer2D_Widths[coords[i]]);
	}

	for (yy = 0; yy < textHeight; yy++) {
		Int32 fontY = srcY + yy * Drawer2D_BoxSize / textHeight;		
		Int32 dstY = y + (yy + yPadding);
		if (dstY >= Drawer2D_Cur->Height) return;

		UInt32* fontRow = Bitmap_GetRow(&Drawer2D_FontBitmap, fontY);
		UInt32* dstRow  = Bitmap_GetRow(Drawer2D_Cur, dstY);
		for (i = 0; i < runCount; i++) {
			Int32 srcX = (coords[i] & 0x0F) * Drawer2D_BoxSize;
			Int32 srcWidth = Drawer2D_Widths[coords[i]], dstWidth = dstWidths[i];

			for (xx = 0; xx < dstWidth; xx++) {
				Int32 fontX = srcX + xx * srcWidth / dstWidth;
				UInt32 src = fontRow[fontX];
				if ((UInt8)(src >> 24) == 0) continue;
				Int32 dstX = x + xx;
				if (dstX >= Drawer2D_Cur->Width) break;

				UInt32 pixel = src & ~0xFFFFFFUL;
				pixel |= (((src)       & 0xFF) * col.B / 255);
				pixel |= (((src >> 8 ) & 0xFF) * col.G / 255) << 8;
				pixel |= (((src >> 16) & 0xFF) * col.R / 255) << 16;
				dstRow[dstX] = pixel;
			}
			x += dstWidth + xPadding;
		}
		x = startX;
	}
}

void Drawer2D_DrawPart(DrawTextArgs* args, Int32 x, Int32 y, bool shadowCol) {
	PackedCol col = Drawer2D_Cols['f'];
	PackedCol black = PACKEDCOL_BLACK;
	if (shadowCol) {
		col = Drawer2D_BlackTextShadows ? black : PackedCol_Scale(col, 0.25f);
	}
	PackedCol lastCol = col;

	Int32 runCount = 0, lastY = -1;
	String text = args->Text;
	Int32 point = args->Font.Size;
	UInt8 coordsPtr[256];

	Int32 i, j;
	for (i = 0; i < text.length; i++) {
		UInt8 c = text.buffer[i];
		if (c == '&' && Drawer2D_ValidColCodeAt(&text, i + 1)) {
			col = Drawer2D_Cols[text.buffer[i + 1]];
			if (shadowCol) {
				col = Drawer2D_BlackTextShadows ? black : PackedCol_Scale(col, 0.25f);
			}
			i++; continue; /* Skip over the colour code */
		}
		Int32 coords = Convert_UnicodeToCP437(c);

		/* First character in the string, begin run counting */
		if (lastY == -1) {
			lastY = coords >> 4; lastCol = col;
			coordsPtr[0] = (UInt8)coords; runCount = 1;
			continue;
		}
		if (lastY == (coords >> 4) && PackedCol_Equals(col, lastCol)) {
			coordsPtr[runCount] = (UInt8)coords; runCount++;
			continue;
		}

		Drawer2D_DrawRun(x, y, runCount, coordsPtr, point, lastCol);
		lastY = coords >> 4; lastCol = col;
		for (j = 0; j < runCount; j++) {
			x += Drawer2D_PaddedWidth(point, Drawer2D_Widths[coordsPtr[j]]);
		}
		coordsPtr[0] = (UInt8)coords; runCount = 1;
	}
	Drawer2D_DrawRun(x, y, runCount, coordsPtr, point, lastCol);
}

void Drawer2D_DrawUnderline(Int32 x, Int32 yOffset, DrawTextArgs* args, bool shadowCol) {
	Int32 point = args->Font.Size;
	Int32 padding = Drawer2D_CellSize(point) - Drawer2D_AdjTextSize(point);
	Int32 height = Drawer2D_AdjTextSize(point) + Math_CeilDiv(padding, 2);
	Int32 offset = Drawer2D_ShadowOffset(args->Font.Size);

	String text = args->Text;
	if (args->UseShadow) height += offset;
	Int32 startX = x;

	Int32 xx, yy, i;
	for (yy = height - offset; yy < height; yy++) {
		UInt32* dstRow = Bitmap_GetRow(Drawer2D_Cur, yy + yOffset);
		UInt32 col = PackedCol_ARGB(255, 255, 255, 255);

		for (i = 0; i < text.length; i++) {
			UInt8 c = text.buffer[i];
			if (c == '&' && Drawer2D_ValidColCodeAt(&text, i + 1)) {
				col = PackedCol_ToARGB(Drawer2D_Cols[text.buffer[i + 1]]);
				i++; continue; // Skip over the colour code.
			}
			if (shadowCol) { col = PackedCol_ARGB(0, 0, 0, 255); }

			Int32 coords = Convert_UnicodeToCP437(c);
			Int32 width = Drawer2D_PaddedWidth(point, Drawer2D_Widths[coords]);
			for (xx = 0; xx < width; xx++) { dstRow[x + xx] = col; }
			x += width;
		}
		x = startX;
	}
}

void Drawer2D_DrawBitmapText(DrawTextArgs* args, Int32 x, Int32 y) {
	bool underline = args->Font.Style == FONT_STYLE_UNDERLINE;
	if (args->UseShadow) {
		Int32 offset = Drawer2D_ShadowOffset(args->Font.Size);
		Drawer2D_DrawPart(args, x + offset, y + offset, true);
		if (underline) Drawer2D_DrawUnderline(x + offset, 0, args, true);
	}

	Drawer2D_DrawPart(args, x, y, false);
	if (underline) Drawer2D_DrawUnderline(x, -2, args, false);
}

Size2D Drawer2D_MeasureBitmapText(DrawTextArgs* args) {
	if (Drawer2D_IsEmptyText(&args->Text)) return Size2D_Empty;
	Int32 textHeight = Drawer2D_AdjTextSize(args->Font.Size);
	Size2D total = { 0, Drawer2D_CellSize(textHeight) };
	Int32 point = Math_Floor(args->Font.Size);

	String text = args->Text;
	Int32 i;
	for (i = 0; i < text.length; i++) {
		UInt8 c = text.buffer[i];
		if (c == '&' && Drawer2D_ValidColCodeAt(&text, i + 1)) {
			i++; continue; /* Skip over the colour code */
		}

		Int32 coords = Convert_UnicodeToCP437(c);
		total.Width += Drawer2D_PaddedWidth(point, Drawer2D_Widths[coords]);
	}

	if (args->UseShadow) {
		Int32 offset = Drawer2D_ShadowOffset(args->Font.Size);
		total.Width += offset; total.Height += offset;
	}
	return total;
}

void Drawer2D_DrawText(DrawTextArgs* args, Int32 x, Int32 y) {
	if (Drawer2D_UseBitmappedChat) {
		Drawer2D_DrawBitmapText(args, x, y);
	} else {
		Platform_DrawText(args, x, y);
	}
}

Size2D Drawer2D_MeasureText(DrawTextArgs* args) {
	if (Drawer2D_UseBitmappedChat) {
		return Drawer2D_MeasureBitmapText(args);
	} else {
		return Platform_MeasureText(args);
	}
}