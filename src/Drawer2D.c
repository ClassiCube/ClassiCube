#include "Drawer2D.h"
#include "Graphics.h"
#include "Funcs.h"
#include "Platform.h"
#include "ExtMath.h"
#include "Logger.h"
#include "Bitmap.h"
#include "Game.h"
#include "Event.h"
#include "Chat.h"

bool Drawer2D_BitmappedText;
bool Drawer2D_BlackTextShadows;
BitmapCol Drawer2D_Cols[DRAWER2D_MAX_COLS];

static char fontNameBuffer[STRING_SIZE];
String Drawer2D_FontName = String_FromArray(fontNameBuffer);

void DrawTextArgs_Make(struct DrawTextArgs* args, STRING_REF const String* text, const FontDesc* font, bool useShadow) {
	args->text = *text;
	args->font = *font;
	args->useShadow = useShadow;
}

void DrawTextArgs_MakeEmpty(struct DrawTextArgs* args, const FontDesc* font, bool useShadow) {
	args->text = String_Empty;
	args->font = *font;
	args->useShadow = useShadow;
}

void Drawer2D_MakeFont(FontDesc* desc, int size, int style) {
	ReturnCode res;
	if (Drawer2D_BitmappedText) {
		desc->Handle = NULL;
		desc->Size   = size;
		desc->Style  = style;
	} else {
		res = Font_Make(desc, &Drawer2D_FontName, size, style);
		if (res) Logger_Abort2(res, "Making default font failed");
	}
}

static Bitmap fontBitmap;
static int tileSize = 8; /* avoid divide by 0 if default.png missing */
/* So really 16 characters per row */
#define LOG2_CHARS_PER_ROW 4
static int tileWidths[256];

static void Drawer2D_CalculateTextWidths(void) {
	int width = fontBitmap.Width, height = fontBitmap.Height;
	BitmapCol* row;
	int i, x, y, xx, tileX, tileY;

	for (y = 0; y < height; y++) {
		tileY = y / tileSize;
		row   = Bitmap_GetRow(&fontBitmap, y);

		for (x = 0; x < width; x += tileSize) {
			tileX = x / tileSize;
			i = tileX | (tileY << LOG2_CHARS_PER_ROW);

			/* Iterate through each pixel of the given character, on the current scanline */
			for (xx = tileSize - 1; xx >= 0; xx--) {
				if (!row[x + xx].A) continue;

				/* Check if this is the pixel furthest to the right, for the current character */			
				tileWidths[i] = max(tileWidths[i], xx + 1);
				break;
			}
		}
	}
	tileWidths[' '] = tileSize / 4;
}

static void Drawer2D_FreeFontBitmap(void) {
	int i;
	for (i = 0; i < Array_Elems(tileWidths); i++) tileWidths[i] = 0;
	Mem_Free(fontBitmap.Scan0);
}

void Drawer2D_SetFontBitmap(Bitmap* bmp) {
	Drawer2D_FreeFontBitmap();
	fontBitmap = *bmp;
	tileSize   = bmp->Width >> LOG2_CHARS_PER_ROW;
	Drawer2D_CalculateTextWidths();
}


bool Drawer2D_Clamp(Bitmap* bmp, int* x, int* y, int* width, int* height) {
	if (*x >= bmp->Width || *y >= bmp->Height) return false;

	/* origin is negative, move inside */
	if (*x < 0) { *width  += *x; *x = 0; }
	if (*y < 0) { *height += *y; *y = 0; }

	*width  = min(*x + *width,  bmp->Width)  - *x;
	*height = min(*y + *height, bmp->Height) - *y;
	return *width > 0 && *height > 0;
}
#define Drawer2D_ClampPixel(p) (p < 0 ? 0 : (p > 255 ? 255 : p))

void Gradient_Noise(Bitmap* bmp, BitmapCol col, int variation,
					int x, int y, int width, int height) {
	BitmapCol* dst;
	int xx, yy, n;
	float noise;
	if (!Drawer2D_Clamp(bmp, &x, &y, &width, &height)) return;

	for (yy = 0; yy < height; yy++) {
		dst = Bitmap_GetRow(bmp, y + yy) + x;

		for (xx = 0; xx < width; xx++, dst++) {
			n = (x + xx) + (y + yy) * 57;
			n = (n << 13) ^ n;
			noise = 1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f;

			n = col.B + (int)(noise * variation);
			dst->B = Drawer2D_ClampPixel(n);
			n = col.G + (int)(noise * variation);
			dst->G = Drawer2D_ClampPixel(n);
			n = col.R + (int)(noise * variation);
			dst->R = Drawer2D_ClampPixel(n);

			dst->A = 255;
		}
	}
}

void Gradient_Vertical(Bitmap* bmp, BitmapCol a, BitmapCol b,
					   int x, int y, int width, int height) {
	BitmapCol* row, col;
	int xx, yy;
	float t;
	if (!Drawer2D_Clamp(bmp, &x, &y, &width, &height)) return;
	col.A = 255;

	for (yy = 0; yy < height; yy++) {
		row = Bitmap_GetRow(bmp, y + yy) + x;
		t   = (float)yy / (height - 1); /* so last row has colour of b */

		col.B = (uint8_t)Math_Lerp(a.B, b.B, t);	
		col.G = (uint8_t)Math_Lerp(a.G, b.G, t);
		col.R = (uint8_t)Math_Lerp(a.R, b.R, t);

		for (xx = 0; xx < width; xx++) { row[xx] = col; }
	}
}

void Gradient_Blend(Bitmap* bmp, BitmapCol col, int blend,
					int x, int y, int width, int height) {
	BitmapCol* dst;
	int xx, yy, t;
	if (!Drawer2D_Clamp(bmp, &x, &y, &width, &height)) return;

	/* Pre compute the alpha blended source colour */
	col.R = (uint8_t)(col.R * blend / 255);
	col.G = (uint8_t)(col.G * blend / 255);
	col.B = (uint8_t)(col.B * blend / 255);
	blend = 255 - blend; /* inverse for existing pixels */

	t = 0;
	for (yy = 0; yy < height; yy++) {
		dst = Bitmap_GetRow(bmp, y + yy) + x;

		for (xx = 0; xx < width; xx++, dst++) {
			t = col.B + (dst->B * blend) / 255;
			dst->B = Drawer2D_ClampPixel(t);
			t = col.G + (dst->G * blend) / 255;
			dst->G = Drawer2D_ClampPixel(t);
			t = col.R + (dst->R * blend) / 255;
			dst->R = Drawer2D_ClampPixel(t);

			dst->A = 255;
		}
	}
}

void Gradient_Tint(Bitmap* bmp, uint8_t tintA, uint8_t tintB,
				   int x, int y, int width, int height) {
	BitmapCol* row;
	uint8_t tint;
	int xx, yy;
	if (!Drawer2D_Clamp(bmp, &x, &y, &width, &height)) return;

	for (yy = 0; yy < height; yy++) {
		row  = Bitmap_GetRow(bmp, y + yy) + x;
		tint = (uint8_t)Math_Lerp(tintA, tintB, (float)yy / height);

		for (xx = 0; xx < width; xx++) {
			row[xx].B = (row[xx].B * tint) / 255;
			row[xx].G = (row[xx].G * tint) / 255;
			row[xx].R = (row[xx].R * tint) / 255;
		}
	}
}

void Drawer2D_BmpIndexed(Bitmap* bmp, int x, int y, int size, 
						uint8_t* indices, BitmapCol* palette) {
	BitmapCol* row;
	BitmapCol col;
	int xx, yy;

	for (yy = 0; yy < size; yy++) {
		if ((y + yy) < 0) { indices += size; continue; }
		if ((y + yy) >= bmp->Height) break;

		row = Bitmap_GetRow(bmp, y + yy) + x;
		for (xx = 0; xx < size; xx++) {
			col = palette[*indices++];

			if (col._raw == 0) continue; /* transparent pixel */
			if ((x + xx) < 0 || (x + xx) >= bmp->Width) continue;
			row[xx] = col;
		}
	}
}

void Drawer2D_BmpCopy(Bitmap* dst, int x, int y, Bitmap* src) {
	int width = src->Width, height = src->Height;
	BitmapCol* dstRow;
	BitmapCol* srcRow;
	int xx, yy;
	if (!Drawer2D_Clamp(dst, &x, &y, &width, &height)) return;

	for (yy = 0; yy < height; yy++) {
		srcRow = Bitmap_GetRow(src, yy);
		dstRow = Bitmap_GetRow(dst, y + yy) + x;

		for (xx = 0; xx < width; xx++) { dstRow[xx] = srcRow[xx]; }
	}
}

void Drawer2D_Clear(Bitmap* bmp, BitmapCol col, int x, int y, int width, int height) {
	BitmapCol* row;
	int xx, yy;
	if (!Drawer2D_Clamp(bmp, &x, &y, &width, &height)) return;

	for (yy = 0; yy < height; yy++) {
		row = Bitmap_GetRow(bmp, y + yy) + x;
		for (xx = 0; xx < width; xx++) { row[xx] = col; }
	}
}


void Drawer2D_MakeTextTexture(struct Texture* tex, struct DrawTextArgs* args, int X, int Y) {
	static struct Texture empty = { GFX_NULL, Tex_Rect(0,0, 0,0), Tex_UV(0,0, 1,1) };
	Size2D size;
	Bitmap bmp;

	size = Drawer2D_MeasureText(args);
	/* height is only 0 when width is 0 */
	if (!size.Width) { *tex = empty; tex->X = X; tex->Y = Y; return; }

	Bitmap_AllocateClearedPow2(&bmp, size.Width, size.Height);
	{
		Drawer2D_DrawText(&bmp, args, 0, 0);
		Drawer2D_Make2DTexture(tex, &bmp, size, X, Y);
	}
	Mem_Free(bmp.Scan0);
}

void Drawer2D_Make2DTexture(struct Texture* tex, Bitmap* bmp, Size2D used, int X, int Y) {
	tex->ID = GFX_NULL;
	if (!Gfx.LostContext) tex->ID = Gfx_CreateTexture(bmp, false, false);

	tex->X  = X; tex->Width  = used.Width;
	tex->Y  = Y; tex->Height = used.Height;

	tex->uv.U1 = 0.0f; tex->uv.V1 = 0.0f;
	tex->uv.U2 = (float)used.Width  / (float)bmp->Width;
	tex->uv.V2 = (float)used.Height / (float)bmp->Height;
}

bool Drawer2D_ValidColCodeAt(const String* text, int i) {
	if (i >= text->length) return false;
	return Drawer2D_GetCol(text->buffer[i]).A > 0;
}

bool Drawer2D_IsEmptyText(const String* text) {
	int i;
	if (!text->length) return true;
	
	for (i = 0; i < text->length; i++) {
		if (text->buffer[i] != '&') return false;
		if (!Drawer2D_ValidColCodeAt(text, i + 1)) return false;
		i++; /* skip colour code */
	}
	return true;
}

char Drawer2D_LastCol(const String* text, int start) {
	int i;
	if (start >= text->length) start = text->length - 1;
	
	for (i = start; i >= 0; i--) {
		if (text->buffer[i] != '&') continue;
		if (Drawer2D_ValidColCodeAt(text, i + 1)) {
			return text->buffer[i + 1];
		}
	}
	return '\0';
}
bool Drawer2D_IsWhiteCol(char c) { return c == '\0' || c == 'f' || c == 'F'; }

/* Divides R/G/B by 4 */
CC_NOINLINE static BitmapCol Drawer2D_ShadowCol(BitmapCol c) {
	c.R >>= 2; c.G >>= 2; c.B >>= 2; 
	return c;
}

#define Drawer2D_ShadowOffset(point) (point / 8)
#define Drawer2D_XPadding(point) (Math_CeilDiv(point, 8))
static int Drawer2D_Width(int point, char c) {
	return Math_CeilDiv(tileWidths[(uint8_t)c] * point, tileSize);
}
static int Drawer2D_AdjHeight(int point) { return Math_CeilDiv(point * 3, 2); }

void Drawer2D_ReducePadding_Tex(struct Texture* tex, int point, int scale) {
	int padding;
	float vAdj;
	if (!Drawer2D_BitmappedText) return;

	padding = (tex->Height - point) / scale;
	vAdj    = (float)padding / Math_NextPowOf2(tex->Height);
	tex->uv.V1 += vAdj; tex->uv.V2 -= vAdj;
	tex->Height -= (uint16_t)(padding * 2);
}

void Drawer2D_ReducePadding_Height(int* height, int point, int scale) {
	int padding;
	if (!Drawer2D_BitmappedText) return;

	padding = (*height - point) / scale;
	*height -= padding * 2;
}

static int Drawer2D_NextPart(int i, STRING_REF String* value, String* part, char* nextCol) {
	int length = 0, start = i;
	for (; i < value->length; i++) {
		if (value->buffer[i] == '&' && Drawer2D_ValidColCodeAt(value, i + 1)) break;
		length++;
	}

	*part = String_UNSAFE_Substring(value, start, length);
	i += 2; /* skip over colour code */

	if (i <= value->length) *nextCol = value->buffer[i - 1];
	return i;
}

void Drawer2D_Underline(Bitmap* bmp, int x, int y, int width, int height, BitmapCol col) {
	BitmapCol* row;
	int xx, yy;

	for (yy = y; yy < y + height; yy++) {
		if (yy >= bmp->Height) return;
		row = Bitmap_GetRow(bmp, yy);

		for (xx = x; xx < x + width; xx++) {
			if (xx >= bmp->Width) break;
			row[xx] = col;
		}
	}
}

static void Drawer2D_DrawCore(Bitmap* bmp, struct DrawTextArgs* args, int x, int y, bool shadow) {
	BitmapCol black = BITMAPCOL_CONST(0, 0, 0, 255);
	BitmapCol col;
	String text  = args->text;
	int i, point = args->font.Size, count = 0;

	int xPadding, yPadding;
	int srcX, srcY, dstX, dstY;
	int fontX, fontY;
	int srcWidth, dstWidth;
	int dstHeight, begX, xx, yy;
	int cellY, underlineY, underlineHeight;

	BitmapCol* srcRow, src;
	BitmapCol* dstRow, dst;

	uint8_t coords[256];
	BitmapCol cols[256];
	uint16_t dstWidths[256];

	col = Drawer2D_Cols['f'];
	if (shadow) {
		col = Drawer2D_BlackTextShadows ? black : Drawer2D_ShadowCol(col);
	}

	for (i = 0; i < text.length; i++) {
		char c = text.buffer[i];
		if (c == '&' && Drawer2D_ValidColCodeAt(&text, i + 1)) {
			col = Drawer2D_GetCol(text.buffer[i + 1]);
			if (shadow) {
				col = Drawer2D_BlackTextShadows ? black : Drawer2D_ShadowCol(col);
			}
			i++; continue; /* skip over the colour code */
		}

		coords[count] = c;
		cols[count]   = col;
		dstWidths[count] = Drawer2D_Width(point, c);
		count++;
	}

	dstHeight = point; begX = x;
	/* adjust coords to make drawn text match GDI fonts */
	xPadding  = Drawer2D_XPadding(point);
	yPadding  = (Drawer2D_AdjHeight(dstHeight) - dstHeight) / 2;

	for (yy = 0; yy < dstHeight; yy++) {
		dstY = y + (yy + yPadding);
		if ((unsigned)dstY >= (unsigned)bmp->Height) continue;

		fontY  = 0 + yy * tileSize / dstHeight;
		dstRow = Bitmap_GetRow(bmp, dstY);

		for (i = 0; i < count; i++) {
			srcX   = (coords[i] & 0x0F) * tileSize;
			srcY   = (coords[i] >> 4)   * tileSize;
			srcRow = Bitmap_GetRow(&fontBitmap, fontY + srcY);

			srcWidth = tileWidths[coords[i]];
			dstWidth = dstWidths[i];
			col      = cols[i];

			for (xx = 0; xx < dstWidth; xx++) {
				fontX = srcX + xx * srcWidth / dstWidth;
				src   = srcRow[fontX];
				if (!src.A) continue;

				dstX = x + xx;
				if ((unsigned)dstX >= (unsigned)bmp->Width) continue;

				dst.B = src.B * col.B / 255;
				dst.G = src.G * col.G / 255;
				dst.R = src.R * col.R / 255;
				dst.A = src.A;
				dstRow[dstX] = dst;
			}
			x += dstWidth + xPadding;
		}
		x = begX;
	}

	if (!(args->font.Style & FONT_FLAG_UNDERLINE)) return;
	/* scale up bottom row of a cell to drawn text font */
	cellY = (8 - 1) * dstHeight / 8;
	underlineY      = y + (cellY + yPadding);
	underlineHeight = dstHeight - cellY;

	for (i = 0; i < count; ) {
		dstWidth = 0;
		col = cols[i];

		for (; i < count && BitmapCol_Equals(col, cols[i]); i++) {
			dstWidth += dstWidths[i] + xPadding;
		}
		Drawer2D_Underline(bmp, x, underlineY, dstWidth, underlineHeight, col);
		x += dstWidth;
	}
}

static void Drawer2D_DrawBitmapText(Bitmap* bmp, struct DrawTextArgs* args, int x, int y) {
	int offset = Drawer2D_ShadowOffset(args->font.Size);

	if (args->useShadow) {
		Drawer2D_DrawCore(bmp, args, x + offset, y + offset, true);
	}
	Drawer2D_DrawCore(bmp, args, x, y, false);
}

static int Drawer2D_MeasureBitmapWidth(const struct DrawTextArgs* args) {
	int i, point = args->font.Size;
	int xPadding, width;
	String text;

	/* adjust coords to make drawn text match GDI fonts */
	xPadding = Drawer2D_XPadding(point);
	width    = 0;

	text = args->text;
	for (i = 0; i < text.length; i++) {
		char c = text.buffer[i];
		if (c == '&' && Drawer2D_ValidColCodeAt(&text, i + 1)) {
			i++; continue; /* skip over the colour code */
		}
		width += Drawer2D_Width(point, c) + xPadding;
	}

	/* TODO: this should be uncommented */
	/* Remove padding at end */
	/*if (width) width -= xPadding; */

	if (args->useShadow) { width += Drawer2D_ShadowOffset(point); }
	return width;
}

void Drawer2D_DrawText(Bitmap* bmp, struct DrawTextArgs* args, int x, int y) {
	BitmapCol col, backCol, black = BITMAPCOL_CONST(0, 0, 0, 255);
	String value = args->text;
	char colCode, nextCol = 'f';
	int i, partWidth;

	if (Drawer2D_IsEmptyText(&args->text)) return;
	if (Drawer2D_BitmappedText) { Drawer2D_DrawBitmapText(bmp, args, x, y); return; }

	for (i = 0; i < value.length; ) {
		colCode = nextCol;
		i = Drawer2D_NextPart(i, &value, &args->text, &nextCol);	
		if (!args->text.length) continue;

		col = Drawer2D_GetCol(colCode);
		if (args->useShadow) {
			backCol = Drawer2D_BlackTextShadows ? black : Drawer2D_ShadowCol(col);
			Platform_TextDraw(args, bmp, x, y, backCol, true);
		}

		partWidth = Platform_TextDraw(args, bmp, x, y, col, false);
		x += partWidth;
	}
	args->text = value;
}

int Drawer2D_TextWidth(struct DrawTextArgs* args) {
	String value = args->text;
	char nextCol = 'f';
	int i, width;

	if (Drawer2D_IsEmptyText(&args->text)) return 0;
	if (Drawer2D_BitmappedText) return Drawer2D_MeasureBitmapWidth(args);
	width = 0;

	for (i = 0; i < value.length; ) {
		i = Drawer2D_NextPart(i, &value, &args->text, &nextCol);

		if (!args->text.length) continue;
		width += Platform_TextWidth(args);
	}

	if (args->useShadow) width += 2;
	args->text = value;
	return width;
}

int Drawer2D_FontHeight(const FontDesc* font, bool useShadow) {
	int height, point;
	if (Drawer2D_BitmappedText) {
		point = font->Size;
		/* adjust coords to make drawn text match GDI fonts */
		height = Drawer2D_AdjHeight(point);

		if (useShadow) { height += Drawer2D_ShadowOffset(point); }
	} else {
		height = Platform_FontHeight(font);
		if (useShadow) height += 2;
	}
	return height;
}

Size2D Drawer2D_MeasureText(struct DrawTextArgs* args) {
	Size2D size;
	size.Width  = Drawer2D_TextWidth(args);
	size.Height = Drawer2D_FontHeight(&args->font, args->useShadow);

	if (!size.Width) size.Height = 0;
	return size;
}

void Drawer2D_DrawClippedText(Bitmap* bmp, struct DrawTextArgs* args, int x, int y, int maxWidth) {
	char strBuffer[512];
	struct DrawTextArgs part;
	int i, width;

	width = Drawer2D_TextWidth(args);
	/* No clipping needed */
	if (width <= maxWidth) { Drawer2D_DrawText(bmp, args, x, y); return; }
	part = *args;

	String_InitArray(part.text, strBuffer);
	String_Copy(&part.text, &args->text);
	String_Append(&part.text, '.');
	
	for (i = part.text.length - 2; i > 0; i--) {
		part.text.buffer[i] = '.';
		/* skip over trailing spaces */
		if (part.text.buffer[i - 1] == ' ') continue;

		part.text.length = i + 2;
		width            = Drawer2D_TextWidth(&part);
		if (width <= maxWidth) { Drawer2D_DrawText(bmp, &part, x, y); return; }

		/* If down to <= 2 chars, try omitting the .. */
		if (i > 2) continue;
		part.text.length = i;
		width            = Drawer2D_TextWidth(&part);
		if (width <= maxWidth) { Drawer2D_DrawText(bmp, &part, x, y); return; }
	}
}


/*########################################################################################################################*
*---------------------------------------------------Drawer2D component----------------------------------------------------*
*#########################################################################################################################*/
static void Drawer2D_HexEncodedCol(int i, int hex, uint8_t lo, uint8_t hi) {
	Drawer2D_Cols[i].R = (uint8_t)(lo * ((hex >> 2) & 1) + hi * (hex >> 3));
	Drawer2D_Cols[i].G = (uint8_t)(lo * ((hex >> 1) & 1) + hi * (hex >> 3));
	Drawer2D_Cols[i].B = (uint8_t)(lo * ((hex >> 0) & 1) + hi * (hex >> 3));
	Drawer2D_Cols[i].A = 255;
}

static void Drawer2D_Reset(void) {
	BitmapCol col = BITMAPCOL_CONST(0, 0, 0, 0);
	int i;
	
	for (i = 0; i < DRAWER2D_MAX_COLS; i++) {
		Drawer2D_Cols[i] = col;
	}

	for (i = 0; i <= 9; i++) {
		Drawer2D_HexEncodedCol('0' + i, i, 191, 64);
	}
	for (i = 10; i <= 15; i++) {
		Drawer2D_HexEncodedCol('a' + (i - 10), i, 191, 64);
		Drawer2D_HexEncodedCol('A' + (i - 10), i, 191, 64);
	}
}

static void Drawer2D_TextureChanged(void* obj, struct Stream* src, const String* name) {
	Bitmap bmp;
	ReturnCode res;
	if (!String_CaselessEqualsConst(name, "default.png")) return;

	if ((res = Png_Decode(&bmp, src))) {
		Logger_Warn2(res, "decoding", name);
		Mem_Free(bmp.Scan0);
	} else {
		Drawer2D_SetFontBitmap(&bmp);
		Event_RaiseVoid(&ChatEvents.FontChanged);
	}
}

static void Drawer2D_CheckFont(void) {
	static const String default_fonts[8] = {
		String_FromConst("Arial"),                      /* preferred font on all platforms */
		String_FromConst("Century Schoolbook L Roman"), /* commonly available on linux */
		String_FromConst("Nimbus Sans"),                /* other common ones */
		String_FromConst("Liberation Sans"),
		String_FromConst("Bitstream Charter"),
		String_FromConst("Cantarell"),
		String_FromConst("DejaVu Sans Book"),
		String_FromConst("Roboto") /* android */
	};
	String path;
	int i;

	path = Font_Lookup(&Drawer2D_FontName, FONT_STYLE_NORMAL);
	if (path.length) return;

	for (i = 0; i < Array_Elems(default_fonts); i++) {
		path = Font_Lookup(&default_fonts[i], FONT_STYLE_NORMAL);
		if (!path.length) continue;

		String_Copy(&Drawer2D_FontName, &default_fonts[i]);
		return;
	}
	Logger_Abort("Unable to init default font");
}

static void Drawer2D_Init(void) {
	Drawer2D_Reset();
	Drawer2D_BitmappedText    = Game_ClassicMode || !Options_GetBool(OPT_USE_CHAT_FONT, false);
	Drawer2D_BlackTextShadows = Options_GetBool(OPT_BLACK_TEXT, false);

	Options_Get(OPT_FONT_NAME, &Drawer2D_FontName, "Arial");
	if (Game_ClassicMode) {
		Drawer2D_FontName.length = 0;
		String_AppendConst(&Drawer2D_FontName, "Arial");
	}

	Drawer2D_CheckFont();
	Event_RegisterEntry(&TextureEvents.FileChanged, NULL, Drawer2D_TextureChanged);
}

static void Drawer2D_Free(void) { 
	Drawer2D_FreeFontBitmap();
	fontBitmap.Scan0 = NULL;
	Event_UnregisterEntry(&TextureEvents.FileChanged, NULL, Drawer2D_TextureChanged);
}

struct IGameComponent Drawer2D_Component = {
	Drawer2D_Init,  /* Init  */
	Drawer2D_Free,  /* Free  */
	Drawer2D_Reset, /* Reset */
};
