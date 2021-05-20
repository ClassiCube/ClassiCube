#include "Drawer2D.h"
#include "String.h"
#include "Graphics.h"
#include "Funcs.h"
#include "Platform.h"
#include "ExtMath.h"
#include "Logger.h"
#include "Game.h"
#include "Event.h"
#include "Chat.h"
#include "Stream.h"
#include "Utils.h"
#include "Errors.h"
#include "Window.h"
#include "Options.h"

struct _Drawer2DData Drawer2D;
#define Font_IsBitmap(font) (!(font)->handle)

void DrawTextArgs_Make(struct DrawTextArgs* args, STRING_REF const cc_string* text, struct FontDesc* font, cc_bool useShadow) {
	args->text = *text;
	args->font = font;
	args->useShadow = useShadow;
}

void DrawTextArgs_MakeEmpty(struct DrawTextArgs* args, struct FontDesc* font, cc_bool useShadow) {
	args->text = String_Empty;
	args->font = font;
	args->useShadow = useShadow;
}


/*########################################################################################################################*
*-----------------------------------------------------Font functions------------------------------------------------------*
*#########################################################################################################################*/
static char defaultBuffer[STRING_SIZE];
static cc_string font_candidates[12] = {
	String_FromArray(defaultBuffer),     /* Filled in with user's default font */
	String_FromConst("Arial"),           /* preferred font on all platforms */
	String_FromConst("Liberation Sans"), /* nice looking fallbacks for linux */
	String_FromConst("Nimbus Sans"),
	String_FromConst("Bitstream Charter"),
	String_FromConst("Cantarell"),
	String_FromConst("DejaVu Sans Book"), 
	String_FromConst("Century Schoolbook L Roman"), /* commonly available on linux */
	String_FromConst("Slate For OnePlus"), /* Android 10, some devices */
	String_FromConst("Roboto"), /* Android (broken on some Android 10 devices) */
	String_FromConst("Geneva"), /* for ancient macOS versions */
	String_FromConst("Droid Sans") /* for old Android versions */
};

void Drawer2D_SetDefaultFont(const cc_string* fontName) {
	String_Copy(&font_candidates[0], fontName);
	Event_RaiseVoid(&ChatEvents.FontChanged);
}

/* adjusts height to be closer to system fonts */
static int Drawer2D_AdjHeight(int point) { return Math_CeilDiv(point * 3, 2); }
void Drawer2D_MakeBitmappedFont(struct FontDesc* desc, int size, int flags) {
	/* TODO: Scale X and Y independently */
	size = Display_ScaleY(size);
	desc->handle = NULL;
	desc->size   = size;
	desc->flags  = flags;
	desc->height = Drawer2D_AdjHeight(size);
}

void Drawer2D_MakeFont(struct FontDesc* desc, int size, int flags) {
	if (Drawer2D.BitmappedText) {
		Drawer2D_MakeBitmappedFont(desc, size, flags);
	} else {
		Font_MakeDefault(desc, size, flags);
	}
}

static struct Bitmap fontBitmap;
static int tileSize = 8; /* avoid divide by 0 if default.png missing */
/* So really 16 characters per row */
#define LOG2_CHARS_PER_ROW 4
static int tileWidths[256];

/* Finds the right-most non-transparent pixel in each tile in default.png */
static void CalculateTextWidths(void) {
	int width = fontBitmap.width, height = fontBitmap.height;
	BitmapCol* row;
	int i, x, y, xx, tileY;

	for (y = 0; y < height; y++) {
		tileY = y / tileSize;
		row   = Bitmap_GetRow(&fontBitmap, y);
		i     = 0 | (tileY << LOG2_CHARS_PER_ROW);

		/* Iterate through each tile on current scanline */
		for (x = 0; x < width; x += tileSize, i++) {
			/* Iterate through each pixel of the given character, on the current scanline */
			for (xx = tileSize - 1; xx >= 0; xx--) {
				if (!BitmapCol_A(row[x + xx])) continue;

				/* Check if this is the pixel furthest to the right, for the current character */
				tileWidths[i] = max(tileWidths[i], xx + 1);
				break;
			}
		}
	}
	tileWidths[' '] = tileSize / 4;
}

static void FreeFontBitmap(void) {
	int i;
	for (i = 0; i < Array_Elems(tileWidths); i++) tileWidths[i] = 0;
	Mem_Free(fontBitmap.scan0);
}

cc_bool Drawer2D_SetFontBitmap(struct Bitmap* bmp) {
	/* If not all of these cases are accounted for, end up overwriting memory after tileWidths */
	if (bmp->width != bmp->height) {
		static const cc_string msg = String_FromConst("&cWidth of default.png must equal its height");
		Logger_WarnFunc(&msg);
		return false;
	} else if (bmp->width < 16) {
		static const cc_string msg = String_FromConst("&cdefault.png must be at least 16 pixels wide");
		Logger_WarnFunc(&msg);
		return false;
	} else if (!Math_IsPowOf2(bmp->width)) {
		static const cc_string msg = String_FromConst("&cWidth of default.png must be a power of two");
		Logger_WarnFunc(&msg);
		return false;
	}

	/* TODO: Use shift instead of mul/div */
	FreeFontBitmap();
	fontBitmap = *bmp;
	tileSize   = bmp->width >> LOG2_CHARS_PER_ROW;

	CalculateTextWidths();
	return true;
}

void Font_SetPadding(struct FontDesc* desc, int amount) {
	if (!Font_IsBitmap(desc)) return;
	desc->height = desc->size + Display_ScaleY(amount) * 2;
}

/* Measures width of the given text when drawn with the given system font. */
static int Font_SysTextWidth(struct DrawTextArgs* args);
/* Draws the given text with the given system font onto the given bitmap. */
static void Font_SysTextDraw(struct DrawTextArgs* args, struct Bitmap* bmp, int x, int y, cc_bool shadow);


/*########################################################################################################################*
*---------------------------------------------------Drawing functions-----------------------------------------------------*
*#########################################################################################################################*/
cc_bool Drawer2D_Clamp(struct Bitmap* bmp, int* x, int* y, int* width, int* height) {
	if (*x >= bmp->width || *y >= bmp->height) return false;

	/* origin is negative, move inside */
	if (*x < 0) { *width  += *x; *x = 0; }
	if (*y < 0) { *height += *y; *y = 0; }

	*width  = min(*x + *width,  bmp->width)  - *x;
	*height = min(*y + *height, bmp->height) - *y;
	return *width > 0 && *height > 0;
}
#define Drawer2D_ClampPixel(p) p = (p < 0 ? 0 : (p > 255 ? 255 : p))

void Gradient_Noise(struct Bitmap* bmp, BitmapCol col, int variation,
					int x, int y, int width, int height) {
	BitmapCol* dst;
	int R, G, B, xx, yy, n;
	float noise;
	if (!Drawer2D_Clamp(bmp, &x, &y, &width, &height)) return;

	for (yy = 0; yy < height; yy++) {
		dst = Bitmap_GetRow(bmp, y + yy) + x;

		for (xx = 0; xx < width; xx++, dst++) {
			n = (x + xx) + (y + yy) * 57;
			n = (n << 13) ^ n;
			noise = 1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f;

			R = BitmapCol_R(col) + (int)(noise * variation); Drawer2D_ClampPixel(R);
			G = BitmapCol_G(col) + (int)(noise * variation); Drawer2D_ClampPixel(G);
			B = BitmapCol_B(col) + (int)(noise * variation); Drawer2D_ClampPixel(B);

			*dst = BitmapCol_Make(R, G, B, 255);
		}
	}
}

void Gradient_Vertical(struct Bitmap* bmp, BitmapCol a, BitmapCol b,
					   int x, int y, int width, int height) {
	BitmapCol* row, col;
	int xx, yy;
	float t;
	if (!Drawer2D_Clamp(bmp, &x, &y, &width, &height)) return;

	for (yy = 0; yy < height; yy++) {
		row = Bitmap_GetRow(bmp, y + yy) + x;
		t   = (float)yy / (height - 1); /* so last row has colour of b */

		col = BitmapCol_Make(
			Math_Lerp(BitmapCol_R(a), BitmapCol_R(b), t),
			Math_Lerp(BitmapCol_G(a), BitmapCol_G(b), t),
			Math_Lerp(BitmapCol_B(a), BitmapCol_B(b), t),
			255);

		for (xx = 0; xx < width; xx++) { row[xx] = col; }
	}
}

void Gradient_Blend(struct Bitmap* bmp, BitmapCol col, int blend,
					int x, int y, int width, int height) {
	BitmapCol* dst;
	int R, G, B, xx, yy;
	if (!Drawer2D_Clamp(bmp, &x, &y, &width, &height)) return;

	/* Pre compute the alpha blended source colour */
	/* TODO: Avoid shift when multiplying */
	col = BitmapCol_Make(
		BitmapCol_R(col) * blend / 255,
		BitmapCol_G(col) * blend / 255,
		BitmapCol_B(col) * blend / 255,
		0);
	blend = 255 - blend; /* inverse for existing pixels */

	for (yy = 0; yy < height; yy++) {
		dst = Bitmap_GetRow(bmp, y + yy) + x;

		for (xx = 0; xx < width; xx++, dst++) {
			/* TODO: Not shift when multiplying */
			R = BitmapCol_R(col) + (BitmapCol_R(*dst) * blend) / 255;
			G = BitmapCol_G(col) + (BitmapCol_G(*dst) * blend) / 255;
			B = BitmapCol_B(col) + (BitmapCol_B(*dst) * blend) / 255;

			*dst = BitmapCol_Make(R, G, B, 255);
		}
	}
}

void Gradient_Tint(struct Bitmap* bmp, cc_uint8 tintA, cc_uint8 tintB,
				   int x, int y, int width, int height) {
	BitmapCol* row, col;
	cc_uint8 tint;
	int xx, yy;
	if (!Drawer2D_Clamp(bmp, &x, &y, &width, &height)) return;

	for (yy = 0; yy < height; yy++) {
		row  = Bitmap_GetRow(bmp, y + yy) + x;
		tint = (cc_uint8)Math_Lerp(tintA, tintB, (float)yy / height);

		for (xx = 0; xx < width; xx++) {
			/* TODO: Not shift when multiplying */
			col = BitmapCol_Make(
				BitmapCol_R(row[xx]) * tint / 255,
				BitmapCol_G(row[xx]) * tint / 255,
				BitmapCol_B(row[xx]) * tint / 255,
				0);

			row[xx] = col | (row[xx] & BITMAPCOL_A_MASK);
		}
	}
}

void Drawer2D_BmpCopy(struct Bitmap* dst, int x, int y, struct Bitmap* src) {
	int width = src->width, height = src->height;
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

void Drawer2D_Clear(struct Bitmap* bmp, BitmapCol col, 
					int x, int y, int width, int height) {
	BitmapCol* row;
	int xx, yy;
	if (!Drawer2D_Clamp(bmp, &x, &y, &width, &height)) return;

	for (yy = 0; yy < height; yy++) {
		row = Bitmap_GetRow(bmp, y + yy) + x;
		for (xx = 0; xx < width; xx++) { row[xx] = col; }
	}
}


void Drawer2D_MakeTextTexture(struct Texture* tex, struct DrawTextArgs* args) {
	static struct Texture empty = { 0, Tex_Rect(0,0, 0,0), Tex_UV(0,0, 1,1) };
	struct Bitmap bmp;
	int width, height;
	/* pointless to draw anything when context is lost */
	if (Gfx.LostContext) { *tex = empty; return; }

	width  = Drawer2D_TextWidth(args);
	if (!width) { *tex = empty; return; }
	height = Drawer2D_TextHeight(args);

	Bitmap_AllocateClearedPow2(&bmp, width, height);
	{
		Drawer2D_DrawText(&bmp, args, 0, 0);
		Drawer2D_MakeTexture(tex, &bmp, width, height);
	}
	Mem_Free(bmp.scan0);
}

void Drawer2D_MakeTexture(struct Texture* tex, struct Bitmap* bmp, int width, int height) {
	Gfx_RecreateTexture(&tex->ID, bmp, false, false);
	tex->Width  = width;
	tex->Height = height;

	tex->uv.U1 = 0.0f; tex->uv.V1 = 0.0f;
	tex->uv.U2 = (float)width  / (float)bmp->width;
	tex->uv.V2 = (float)height / (float)bmp->height;
}

cc_bool Drawer2D_ValidColCodeAt(const cc_string* text, int i) {
	if (i >= text->length) return false;
	return BitmapCol_A(Drawer2D_GetCol(text->buffer[i])) != 0;
}

cc_bool Drawer2D_IsEmptyText(const cc_string* text) {
	int i;
	if (!text->length) return true;
	
	for (i = 0; i < text->length; i++) {
		if (text->buffer[i] != '&') return false;
		if (!Drawer2D_ValidColCodeAt(text, i + 1)) return false;
		i++; /* skip colour code */
	}
	return true;
}

void Drawer2D_WithoutCols(cc_string* str, const cc_string* src) {
	char c;
	int i;
	for (i = 0; i < src->length; i++) {
		c  = src->buffer[i];

		if (c == '&' && Drawer2D_ValidColCodeAt(src, i + 1)) {
			i++; /* skip colour code */
		} else {
			String_Append(str, c);
		}	
	}
}

char Drawer2D_LastCol(const cc_string* text, int start) {
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
cc_bool Drawer2D_IsWhiteCol(char c) { return c == '\0' || c == 'f' || c == 'F'; }

/* Divides R/G/B by 4 */
#define SHADOW_MASK ((0x3F << BITMAPCOL_R_SHIFT) | (0x3F << BITMAPCOL_G_SHIFT) | (0x3F << BITMAPCOL_B_SHIFT))
CC_NOINLINE static BitmapCol GetShadowCol(BitmapCol c) {
	if (Drawer2D.BlackTextShadows) return BITMAPCOL_BLACK;

	/* Initial layout: aaaa_aaaa|rrrr_rrrr|gggg_gggg|bbbb_bbbb */
	/* Shift right 2:  00aa_aaaa|aarr_rrrr|rrgg_gggg|ggbb_bbbb */
	/* And by 3f3f3f:  0000_0000|00rr_rrrr|00gg_gggg|00bb_bbbb */
	/* Or by alpha  :  aaaa_aaaa|00rr_rrrr|00gg_gggg|00bb_bbbb */
	return (c & BITMAPCOL_A_MASK) | ((c >> 2) & SHADOW_MASK);
}

/* TODO: Needs to account for DPI */
#define Drawer2D_ShadowOffset(point) (point / 8)
#define Drawer2D_XPadding(point) (Math_CeilDiv(point, 8))
static int Drawer2D_Width(int point, char c) {
	return Math_CeilDiv(tileWidths[(cc_uint8)c] * point, tileSize);
}

void Drawer2D_ReducePadding_Tex(struct Texture* tex, int point, int scale) {
	int padding;
	float vAdj;
	if (!Drawer2D.BitmappedText) return;

	padding = (tex->Height - point) / scale;
	vAdj    = (float)padding / Math_NextPowOf2(tex->Height);
	tex->uv.V1 += vAdj; tex->uv.V2 -= vAdj;
	tex->Height -= (cc_uint16)(padding * 2);
}

void Drawer2D_ReducePadding_Height(int* height, int point, int scale) {
	int padding;
	if (!Drawer2D.BitmappedText) return;

	padding = (*height - point) / scale;
	*height -= padding * 2;
}

/* Quickly fills the given box region */
static void Drawer2D_Underline(struct Bitmap* bmp, int x, int y, int width, int height, BitmapCol col) {
	BitmapCol* row;
	int xx, yy;

	for (yy = y; yy < y + height; yy++) {
		if (yy >= bmp->height) return;
		row = Bitmap_GetRow(bmp, yy);

		for (xx = x; xx < x + width; xx++) {
			if (xx >= bmp->width) break;
			row[xx] = col;
		}
	}
}

static void DrawBitmappedTextCore(struct Bitmap* bmp, struct DrawTextArgs* args, int x, int y, cc_bool shadow) {
	BitmapCol col;
	cc_string text = args->text;
	int i, point   = args->font->size, count = 0;

	int xPadding;
	int srcX, srcY, dstX, dstY;
	int fontX, fontY;
	int srcWidth, dstWidth;
	int dstHeight, begX, xx, yy;
	int cellY, underlineY, underlineHeight;

	BitmapCol* srcRow, src;
	BitmapCol* dstRow;

	cc_uint8 coords[256];
	BitmapCol cols[256];
	cc_uint16 dstWidths[256];

	col = Drawer2D.Colors['f'];
	if (shadow) col = GetShadowCol(col);

	for (i = 0; i < text.length; i++) {
		char c = text.buffer[i];
		if (c == '&' && Drawer2D_ValidColCodeAt(&text, i + 1)) {
			col = Drawer2D_GetCol(text.buffer[i + 1]);

			if (shadow) col = GetShadowCol(col);
			i++; continue; /* skip over the colour code */
		}

		coords[count] = c;
		cols[count]   = col;
		dstWidths[count] = Drawer2D_Width(point, c);
		count++;
	}

	dstHeight = point; begX = x;
	/* adjust coords to make drawn text match GDI fonts */
	y += (args->font->height - dstHeight) / 2;
	xPadding  = Drawer2D_XPadding(point);

	for (yy = 0; yy < dstHeight; yy++) {
		dstY = y + yy;
		if ((unsigned)dstY >= (unsigned)bmp->height) continue;

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
				if (!BitmapCol_A(src)) continue;

				dstX = x + xx;
				if ((unsigned)dstX >= (unsigned)bmp->width) continue;

				/* TODO: Transparent text by multiplying by col.A */
				/* TODO: Not shift when multiplying */
				/* TODO: avoid BitmapCol_A shift */
				dstRow[dstX] = BitmapCol_Make(
					BitmapCol_R(src) * BitmapCol_R(col) / 255,
					BitmapCol_G(src) * BitmapCol_G(col) / 255,
					BitmapCol_B(src) * BitmapCol_B(col) / 255,
					BitmapCol_A(src));
			}
			x += dstWidth + xPadding;
		}
		x = begX;
	}

	if (!(args->font->flags & FONT_FLAGS_UNDERLINE)) return;
	/* scale up bottom row of a cell to drawn text font */
	cellY = (8 - 1) * dstHeight / 8;
	underlineY      = y + cellY;
	underlineHeight = dstHeight - cellY;

	for (i = 0; i < count; ) {
		dstWidth = 0;
		col = cols[i];

		for (; i < count && col == cols[i]; i++) {
			dstWidth += dstWidths[i] + xPadding;
		}
		Drawer2D_Underline(bmp, x, underlineY, dstWidth, underlineHeight, col);
		x += dstWidth;
	}
}

static void DrawBitmappedText(struct Bitmap* bmp, struct DrawTextArgs* args, int x, int y) {
	int offset = Drawer2D_ShadowOffset(args->font->size);

	if (args->useShadow) {
		DrawBitmappedTextCore(bmp, args, x + offset, y + offset, true);
	}
	DrawBitmappedTextCore(bmp, args, x, y, false);
}

static int MeasureBitmappedWidth(const struct DrawTextArgs* args) {
	int i, point = args->font->size;
	int xPadding, width;
	cc_string text;

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

	/* Remove padding at end */
	if (width && !(args->font->flags & FONT_FLAGS_PADDING)) width -= xPadding;

	if (args->useShadow) { width += Drawer2D_ShadowOffset(point); }
	return width;
}

void Drawer2D_DrawText(struct Bitmap* bmp, struct DrawTextArgs* args, int x, int y) {
	if (Drawer2D_IsEmptyText(&args->text)) return;
	if (Font_IsBitmap(args->font)) { DrawBitmappedText(bmp, args, x, y); return; }

	if (args->useShadow) { Font_SysTextDraw(args, bmp, x, y, true); }
	Font_SysTextDraw(args, bmp, x, y, false);
}

int Drawer2D_TextWidth(struct DrawTextArgs* args) {
	if (Drawer2D_IsEmptyText(&args->text)) return 0;
	if (Font_IsBitmap(args->font)) return MeasureBitmappedWidth(args);
	return Font_SysTextWidth(args);
}

int Drawer2D_TextHeight(struct DrawTextArgs* args) {
	return Drawer2D_FontHeight(args->font, args->useShadow);
}

int Drawer2D_FontHeight(const struct FontDesc* font, cc_bool useShadow) {
	int height = font->height;
	if (Font_IsBitmap(font)) {
		if (useShadow) { height += Drawer2D_ShadowOffset(font->size); }
	} else {
		if (useShadow) height += 2;
	}
	return height;
}

void Drawer2D_DrawClippedText(struct Bitmap* bmp, struct DrawTextArgs* args, 
								int x, int y, int maxWidth) {
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
static void InitHexEncodedCol(int i, int hex, cc_uint8 lo, cc_uint8 hi) {
	Drawer2D.Colors[i] = BitmapCol_Make(
		lo * ((hex >> 2) & 1) + hi * (hex >> 3),
		lo * ((hex >> 1) & 1) + hi * (hex >> 3),
		lo * ((hex >> 0) & 1) + hi * (hex >> 3),
		255);
}

static void OnReset(void) {
	int i;	
	for (i = 0; i < DRAWER2D_MAX_COLS; i++) {
		Drawer2D.Colors[i] = 0;
	}

	for (i = 0; i <= 9; i++) {
		InitHexEncodedCol('0' + i, i, 191, 64);
	}
	for (i = 10; i <= 15; i++) {
		InitHexEncodedCol('a' + (i - 10), i, 191, 64);
		InitHexEncodedCol('A' + (i - 10), i, 191, 64);
	}
}

static void OnFileChanged(void* obj, struct Stream* src, const cc_string* name) {
	struct Bitmap bmp;
	cc_result res;
	if (!String_CaselessEqualsConst(name, "default.png")) return;

	if ((res = Png_Decode(&bmp, src))) {
		Logger_SysWarn2(res, "decoding", name);
		Mem_Free(bmp.scan0);
	} else if (Drawer2D_SetFontBitmap(&bmp)) {
		Event_RaiseVoid(&ChatEvents.FontChanged);
	} else {
		Mem_Free(bmp.scan0);
	}
}

static void OnInit(void) {
	OnReset();
	Drawer2D.BitmappedText    = Game_ClassicMode || !Options_GetBool(OPT_USE_CHAT_FONT, false);
	Drawer2D.BlackTextShadows = Options_GetBool(OPT_BLACK_TEXT, false);

	Options_Get(OPT_FONT_NAME, &font_candidates[0], "");
	if (Game_ClassicMode) font_candidates[0].length = 0;
	Event_Register_(&TextureEvents.FileChanged, NULL, OnFileChanged);
}

static void OnFree(void) { 
	FreeFontBitmap();
	fontBitmap.scan0 = NULL;
}

struct IGameComponent Drawer2D_Component = {
	OnInit,  /* Init  */
	OnFree,  /* Free  */
	OnReset, /* Reset */
};


/*########################################################################################################################*
*------------------------------------------------------System fonts-------------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_WEB
const cc_string* Font_UNSAFE_GetDefault(void) { return &font_candidates[0]; }
void Font_GetNames(struct StringsBuffer* buffer) { }
cc_string Font_Lookup(const cc_string* fontName, int flags) { return String_Empty; }

cc_result Font_Make(struct FontDesc* desc, const cc_string* fontName, int size, int flags) {
	desc->size   = size;
	desc->flags  = flags;
	desc->height = 0;
	return 0;
}
void Font_MakeDefault(struct FontDesc* desc, int size, int flags) { Font_Make(desc, NULL, size, flags); }

void Font_Free(struct FontDesc* desc) {
	desc->size   = 0;
}

void SysFonts_Register(const cc_string* path) { }
static int Font_SysTextWidth(struct DrawTextArgs* args) { return 0; }
static void Font_SysTextDraw(struct DrawTextArgs* args, struct Bitmap* bmp, int x, int y, cc_bool shadow) { }
#else
#include "freetype/ft2build.h"
#include "freetype/freetype.h"
#include "freetype/ftmodapi.h"
#include "freetype/ftglyph.h"

static FT_Library ft_lib;
static struct FT_MemoryRec_ ft_mem;
static struct StringsBuffer font_list;
static cc_bool fonts_changed;

struct SysFont {
	FT_Face face;
	struct Stream src, file;
	FT_StreamRec stream;
	cc_uint8 buffer[8192]; /* small buffer to minimise disk I/O */
	cc_uint16 widths[256]; /* cached width of each character glyph */
	FT_BitmapGlyph glyphs[256];        /* cached glyphs */
	FT_BitmapGlyph shadow_glyphs[256]; /* cached glyphs (for back layer shadow) */
#ifdef CC_BUILD_DARWIN
	char filename[FILENAME_SIZE + 1];
#endif
};

static unsigned long SysFont_Read(FT_Stream s, unsigned long offset, unsigned char* buffer, unsigned long count) {
	struct SysFont* font;
	cc_result res;
	if (!count && offset > s->size) return 1;

	font = (struct SysFont*)s->descriptor.pointer;
	if (s->pos != offset) font->src.Seek(&font->src, offset);

	res = Stream_Read(&font->src, buffer, count);
	return res ? 0 : count;
}

static void SysFont_Free(struct SysFont* font) {
	int i;

	/* Close the actual underlying file */
	struct Stream* source = &font->file;
	if (!source->Meta.File) return;
	source->Close(source);

	for (i = 0; i < 256; i++) {
		if (!font->glyphs[i]) continue;
		FT_Done_Glyph((FT_Glyph)font->glyphs[i]);
	}
	for (i = 0; i < 256; i++) {
		if (!font->shadow_glyphs[i]) continue;
		FT_Done_Glyph((FT_Glyph)font->shadow_glyphs[i]);
	}
}

static void SysFont_Close(FT_Stream stream) {
	struct SysFont* font = (struct SysFont*)stream->descriptor.pointer;
	SysFont_Free(font);
}

static cc_result SysFont_Init(const cc_string* path, struct SysFont* font, FT_Open_Args* args) {
	cc_file file;
	cc_uint32 size;
	cc_result res;
#ifdef CC_BUILD_DARWIN
	cc_string filename;
#endif

	if ((res = File_Open(&file, path))) return res;
	if ((res = File_Length(file, &size))) { File_Close(file); return res; }

	font->stream.base = NULL;
	font->stream.size = size;
	font->stream.pos  = 0;

	font->stream.descriptor.pointer = font;
	font->stream.read   = SysFont_Read;
	font->stream.close  = SysFont_Close;

	font->stream.memory = &ft_mem;
	font->stream.cursor = NULL;
	font->stream.limit  = NULL;

	args->flags    = FT_OPEN_STREAM;
	args->pathname = NULL;
	args->stream   = &font->stream;

	Stream_FromFile(&font->file, file);
	Stream_ReadonlyBuffered(&font->src, &font->file, font->buffer, sizeof(font->buffer));

	/* For OSX font suitcase files */
#ifdef CC_BUILD_DARWIN
	String_InitArray_NT(filename, font->filename);
	String_Copy(&filename, path);
	font->filename[filename.length] = '\0';
	args->pathname = font->filename;
#endif
	Mem_Set(font->widths,        0xFF, sizeof(font->widths));
	Mem_Set(font->glyphs,        0x00, sizeof(font->glyphs));
	Mem_Set(font->shadow_glyphs, 0x00, sizeof(font->shadow_glyphs));
	return 0;
}

static void* FT_AllocWrapper(FT_Memory memory, long size) { return Mem_TryAlloc(size, 1); }
static void FT_FreeWrapper(FT_Memory memory, void* block) { Mem_Free(block); }
static void* FT_ReallocWrapper(FT_Memory memory, long cur_size, long new_size, void* block) {
	return Mem_TryRealloc(block, new_size, 1);
}

#define FONT_CACHE_FILE "fontscache.txt"
static void SysFonts_InitLibrary(void) {
	FT_Error err;
	if (ft_lib) return;

	ft_mem.alloc   = FT_AllocWrapper;
	ft_mem.free    = FT_FreeWrapper;
	ft_mem.realloc = FT_ReallocWrapper;

	err = FT_New_Library(&ft_mem, &ft_lib);
	if (err) Logger_Abort2(err, "Failed to init freetype");
	FT_Add_Default_Modules(ft_lib);
}

/* Updates fonts list cache with system's list of fonts */
/* This should be avoided due to overhead potential */
static void SysFonts_Update(void) {
	static cc_bool updatedFonts;
	if (updatedFonts) return;
	updatedFonts = true;

	SysFonts_InitLibrary();
	Platform_LoadSysFonts();
	if (fonts_changed) EntryList_Save(&font_list, FONT_CACHE_FILE);
}

static void SysFonts_Load(void) {
	EntryList_UNSAFE_Load(&font_list, FONT_CACHE_FILE);
	if (font_list.count) return;
	
	Window_ShowDialog("One time load", "Initialising font cache, this can take several seconds.");
	SysFonts_Update();
}

/* Some language-specific fonts don't support English letters */
/* and show entirely as '[]' - better off ignoring such fonts */
static cc_bool SysFonts_SkipFont(FT_Face face) {
	if (!face->charmap) return false;

	return FT_Get_Char_Index(face, 'a') == 0 && FT_Get_Char_Index(face, 'z') == 0
		&& FT_Get_Char_Index(face, 'A') == 0 && FT_Get_Char_Index(face, 'Z') == 0;
}

static void SysFonts_Add(const cc_string* path, FT_Face face, int index, char type, const char* defStyle) {
	cc_string key;   char keyBuffer[STRING_SIZE];
	cc_string value; char valueBuffer[FILENAME_SIZE];
	cc_string style = String_Empty;

	if (!face->family_name || !(face->face_flags & FT_FACE_FLAG_SCALABLE)) return;
	/* don't want 'Arial Regular' or 'Arial Bold' */
	if (face->style_name) {
		style = String_FromReadonly(face->style_name);
		if (String_CaselessEqualsConst(&style, defStyle)) style.length = 0;
	}
	if (SysFonts_SkipFont(face)) type = 'X';

	String_InitArray(key, keyBuffer);
	if (style.length) {
		String_Format3(&key, "%c %c %r", face->family_name, face->style_name, &type);
	} else {
		String_Format2(&key, "%c %r", face->family_name, &type);
	}

	String_InitArray(value, valueBuffer);
	String_Format2(&value, "%s,%i", path, &index);

	Platform_Log2("Face: %s = %s", &key, &value);
	EntryList_Set(&font_list, &key, &value, '=');
	fonts_changed = true;
}

static int SysFonts_DoRegister(const cc_string* path, int faceIndex) {
	struct SysFont font;
	FT_Open_Args args;
	FT_Error err;
	int flags, count;

	if (SysFont_Init(path, &font, &args)) return 0;
	err = FT_New_Face(ft_lib, &args, faceIndex, &font.face);
	if (err) { SysFont_Free(&font); return 0; }

	flags = font.face->style_flags;
	count = font.face->num_faces;

	if (flags == (FT_STYLE_FLAG_BOLD | FT_STYLE_FLAG_ITALIC)) {
		SysFonts_Add(path, font.face, faceIndex, 'Z', "Bold Italic");
	} else if (flags == FT_STYLE_FLAG_BOLD) {
		SysFonts_Add(path, font.face, faceIndex, 'B', "Bold");
	} else if (flags == FT_STYLE_FLAG_ITALIC) {
		SysFonts_Add(path, font.face, faceIndex, 'I', "Italic");
	} else if (flags == 0) {
		SysFonts_Add(path, font.face, faceIndex, 'R', "Regular");
	}

	FT_Done_Face(font.face);
	return count;
}

void SysFonts_Register(const cc_string* path) {
	cc_string entry, name, value;
	cc_string fontPath, index;
	int i, count;

	/* if font is already known, skip it */
	for (i = 0; i < font_list.count; i++) {
		StringsBuffer_UNSAFE_GetRaw(&font_list, i, &entry);
		String_UNSAFE_Separate(&entry, '=', &name, &value);

		String_UNSAFE_Separate(&value, ',', &fontPath, &index);
		if (String_CaselessEquals(path, &fontPath)) return;
	}

	count = SysFonts_DoRegister(path, 0);
	/* there may be more than one font in a font file */
	for (i = 1; i < count; i++) {
		SysFonts_DoRegister(path, i);
	}
}

const cc_string* Font_UNSAFE_GetDefault(void) {
	cc_string* font, path;
	int i;

	for (i = 0; i < Array_Elems(font_candidates); i++) {
		font = &font_candidates[i];
		if (!font->length) continue;

		path = Font_Lookup(font, FONT_FLAGS_NONE);
		if (path.length) return font;
	}
	return &String_Empty;
}

void Font_GetNames(struct StringsBuffer* buffer) {
	cc_string entry, name, path;
	int i;
	if (!font_list.count) SysFonts_Load();
	SysFonts_Update();

	for (i = 0; i < font_list.count; i++) {
		StringsBuffer_UNSAFE_GetRaw(&font_list, i, &entry);
		String_UNSAFE_Separate(&entry, '=', &name, &path);

		/* only want Regular fonts here */
		if (name.length < 2 || name.buffer[name.length - 1] != 'R') continue;
		name.length -= 2;
		StringsBuffer_Add(buffer, &name);
	}
}

static cc_string Font_LookupOf(const cc_string* fontName, const char type) {
	cc_string name; char nameBuffer[STRING_SIZE + 2];
	String_InitArray(name, nameBuffer);

	String_Format2(&name, "%s %r", fontName, &type);
	return EntryList_UNSAFE_Get(&font_list, &name, '=');
}

static cc_string Font_DoLookup(const cc_string* fontName, int flags) {
	cc_string path;
	if (!font_list.count) SysFonts_Load();
	path = String_Empty;

	if (flags & FONT_FLAGS_BOLD) path = Font_LookupOf(fontName, 'B');
	return path.length ? path : Font_LookupOf(fontName, 'R');
}

cc_string Font_Lookup(const cc_string* fontName, int flags) {
	cc_string path = Font_DoLookup(fontName, flags);
	if (path.length) return path;

	SysFonts_Update();
	return Font_DoLookup(fontName, flags);
}

#define TEXT_CEIL(x) (((x) + 63) >> 6)
cc_result Font_Make(struct FontDesc* desc, const cc_string* fontName, int size, int flags) {
	struct SysFont* font;
	cc_string value, path, index;
	int faceIndex, dpiX, dpiY;
	FT_Open_Args args;
	FT_Error err;

	desc->size   = size;
	desc->flags  = flags;
	desc->handle = NULL;

	value = Font_Lookup(fontName, flags);
	if (!value.length) return ERR_INVALID_ARGUMENT;
	String_UNSAFE_Separate(&value, ',', &path, &index);
	Convert_ParseInt(&index, &faceIndex);

	font = (struct SysFont*)Mem_TryAlloc(1, sizeof(struct SysFont));
	if (!font) return ERR_OUT_OF_MEMORY;

	SysFonts_InitLibrary();
	if ((err = SysFont_Init(&path, font, &args))) { Mem_Free(font); return err; }
	desc->handle = font;

	/* TODO: Use 72 instead of 96 dpi for mobile devices */
	dpiX = (int)(DisplayInfo.ScaleX * 96);
	dpiY = (int)(DisplayInfo.ScaleY * 96);

	if ((err = FT_New_Face(ft_lib, &args, faceIndex, &font->face)))     return err;
	if ((err = FT_Set_Char_Size(font->face, size * 64, 0, dpiX, dpiY))) return err;

	/* height of any text when drawn with the given system font */
	desc->height = TEXT_CEIL(font->face->size->metrics.height);
	return 0;
}

void Font_MakeDefault(struct FontDesc* desc, int size, int flags) {
	cc_string* font;
	cc_result res;
	int i;

	for (i = 0; i < Array_Elems(font_candidates); i++) {
		font = &font_candidates[i];
		if (!font->length) continue;
		res  = Font_Make(desc, &font_candidates[i], size, flags);

		if (res == ERR_INVALID_ARGUMENT) {
			/* Fon't doesn't exist in list, skip over it */
		} else if (res) {
			Font_Free(desc);
			Logger_SysWarn2(res, "creating font", font);
		} else {
			if (i) String_Copy(&font_candidates[0], font);
			return;
		}
	}
	Logger_Abort2(res, "Failed to init default font");
}

void Font_Free(struct FontDesc* desc) {
	struct SysFont* font;
	desc->size = 0;
	if (Font_IsBitmap(desc)) return;

	font = (struct SysFont*)desc->handle;
	FT_Done_Face(font->face);
	Mem_Free(font);
	desc->handle = NULL;
}

static int Font_SysTextWidth(struct DrawTextArgs* args) {
	struct SysFont* font = (struct SysFont*)args->font->handle;
	FT_Face face   = font->face;
	cc_string text = args->text;
	int i, width = 0, charWidth;
	FT_Error res;
	cc_unichar uc;

	for (i = 0; i < text.length; i++) {
		char c = text.buffer[i];
		if (c == '&' && Drawer2D_ValidColCodeAt(&text, i + 1)) {
			i++; continue; /* skip over the colour code */
		}

		charWidth = font->widths[(cc_uint8)c];
		/* need to calculate glyph width */
		if (charWidth == UInt16_MaxValue) {
			uc  = Convert_CP437ToUnicode(c);
			res = FT_Load_Char(face, uc, 0);

			if (res) {
				Platform_Log2("Error %i measuring width of %r", &res, &c);
				charWidth = 0;
			} else {
				charWidth = face->glyph->advance.x;		
			}

			font->widths[(cc_uint8)c] = charWidth;
		}
		width += charWidth;
	}

	width = TEXT_CEIL(width);
	if (args->useShadow) width += 2;
	return width;
}

static void DrawGrayscaleGlyph(FT_Bitmap* img, struct Bitmap* bmp, int x, int y, BitmapCol col) {
	cc_uint8* src;
	BitmapCol* dst;
	cc_uint8 I, invI; /* intensity */
	int xx, yy;

	for (yy = 0; yy < img->rows; yy++) {
		if ((unsigned)(y + yy) >= (unsigned)bmp->height) continue;
		src = img->buffer + (yy * img->pitch);
		dst = Bitmap_GetRow(bmp, y + yy) + x;

		for (xx = 0; xx < img->width; xx++, src++, dst++) {
			if ((unsigned)(x + xx) >= (unsigned)bmp->width) continue;
			I = *src; invI = UInt8_MaxValue - I;

			/* TODO: Support transparent text */
			/* dst->A = ((col.A * intensity) >> 8) + ((dst->A * invIntensity) >> 8);*/
			/* TODO: Not shift when multiplying */
			*dst = BitmapCol_Make(
				((BitmapCol_R(col) * I) >> 8) + ((BitmapCol_R(*dst) * invI) >> 8),
				((BitmapCol_G(col) * I) >> 8) + ((BitmapCol_G(*dst) * invI) >> 8),
				((BitmapCol_B(col) * I) >> 8) + ((BitmapCol_B(*dst) * invI) >> 8),
				                           I  + ((BitmapCol_A(*dst) * invI) >> 8)
			);
		}
	}
}

static void DrawBlackWhiteGlyph(FT_Bitmap* img, struct Bitmap* bmp, int x, int y, BitmapCol col) {
	cc_uint8* src;
	BitmapCol* dst;
	cc_uint8 intensity;
	int xx, yy;

	for (yy = 0; yy < img->rows; yy++) {
		if ((unsigned)(y + yy) >= (unsigned)bmp->height) continue;
		src = img->buffer + (yy * img->pitch);
		dst = Bitmap_GetRow(bmp, y + yy) + x;

		for (xx = 0; xx < img->width; xx++, dst++) {
			if ((unsigned)(x + xx) >= (unsigned)bmp->width) continue;
			intensity = src[xx >> 3];

			/* TODO: transparent text (don't set A to 255) */
			if (intensity & (1 << (7 - (xx & 7)))) {
				*dst = col | BitmapCol_A_Bits(255);
			}
		}
	}
}

static FT_Vector shadow_delta = { 83, -83 };
static void Font_SysTextDraw(struct DrawTextArgs* args, struct Bitmap* bmp, int x, int y, cc_bool shadow) {
	struct SysFont* font  = (struct SysFont*)args->font->handle;
	FT_BitmapGlyph* glyphs = font->glyphs;

	FT_Face face   = font->face;
	cc_string text = args->text;
	int descender, height, begX = x;
	BitmapCol col;
	
	/* glyph state */
	FT_BitmapGlyph glyph;
	FT_Bitmap* img;
	int i, offset;
	FT_Error res;
	cc_unichar uc;

	if (shadow) {
		glyphs = font->shadow_glyphs;
		FT_Set_Transform(face, NULL, &shadow_delta);
	}

	height    = args->font->height;
	descender = TEXT_CEIL(face->size->metrics.descender);

	col = Drawer2D.Colors['f'];
	if (shadow) col = GetShadowCol(col);

	for (i = 0; i < text.length; i++) {
		char c = text.buffer[i];
		if (c == '&' && Drawer2D_ValidColCodeAt(&text, i + 1)) {
			col = Drawer2D_GetCol(text.buffer[i + 1]);

			if (shadow) col = GetShadowCol(col);
			i++; continue; /* skip over the colour code */
		}

		glyph = glyphs[(cc_uint8)c];
		if (!glyph) {
			uc  = Convert_CP437ToUnicode(c);
			res = FT_Load_Char(face, uc, FT_LOAD_RENDER);

			if (res) {
				Platform_Log2("Error %i drawing %r", &res, &text.buffer[i]);
				continue;
			}

			/* due to FT_LOAD_RENDER, glyph is always a bitmap one */
			FT_Get_Glyph(face->glyph, (FT_Glyph*)&glyph); /* TODO: Check error */
			glyphs[(cc_uint8)c] = glyph;
		}

		offset = (height + descender) - glyph->top;
		x += glyph->left; y += offset;

		img = &glyph->bitmap;
		if (img->num_grays == 2) {
			DrawBlackWhiteGlyph(img, bmp, x, y, col);
		} else {
			DrawGrayscaleGlyph(img, bmp, x, y, col);
		}

		x += TEXT_CEIL(glyph->root.advance.x >> 10);
		x -= glyph->left; y -= offset;
	}

	if (args->font->flags & FONT_FLAGS_UNDERLINE) {
		int ul_pos   = FT_MulFix(face->underline_position,  face->size->metrics.y_scale);
		int ul_thick = FT_MulFix(face->underline_thickness, face->size->metrics.y_scale);

		int ulHeight = TEXT_CEIL(ul_thick);
		int ulY      = height + TEXT_CEIL(ul_pos);
		Drawer2D_Underline(bmp, begX, ulY + y, x - begX, ulHeight, col);
	}

	if (shadow) FT_Set_Transform(face, NULL, NULL);
}
#endif
