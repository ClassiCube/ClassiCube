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
#include "TexturePack.h"
#include "SystemFonts.h"

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
int Drawer2D_AdjHeight(int point) { return Math_CeilDiv(point * 3, 2); }

void Font_MakeBitmapped(struct FontDesc* desc, int size, int flags) {
	/* TODO: Scale X and Y independently */
	size = Display_ScaleY(size);
	desc->handle = NULL;
	desc->size   = size;
	desc->flags  = flags;
	desc->height = Drawer2D_AdjHeight(size);
}

void Font_Make(struct FontDesc* desc, int size, int flags) {
	if (Drawer2D.BitmappedText) {
		Font_MakeBitmapped(desc, size, flags);
	} else {
		SysFont_MakeDefault(desc, size, flags);
	}
}

void Font_Free(struct FontDesc* desc) {
	desc->size = 0;
	if (Font_IsBitmap(desc)) return;

	SysFont_Free(desc);
	desc->handle = NULL;
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

cc_bool Font_SetBitmapAtlas(struct Bitmap* bmp) {
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


/*########################################################################################################################*
*---------------------------------------------------Drawing functions-----------------------------------------------------*
*#########################################################################################################################*/
cc_bool Drawer2D_Clamp(struct Context2D* ctx, int* x, int* y, int* width, int* height) {
	if (*x >= ctx->width || *y >= ctx->height) return false;

	/* origin is negative, move inside */
	if (*x < 0) { *width  += *x; *x = 0; }
	if (*y < 0) { *height += *y; *y = 0; }

	*width  = min(*x + *width,  ctx->width)  - *x;
	*height = min(*y + *height, ctx->height) - *y;
	return *width > 0 && *height > 0;
}
#define Drawer2D_ClampPixel(p) p = (p < 0 ? 0 : (p > 255 ? 255 : p))

void Context2D_Alloc(struct Context2D* ctx, int width, int height) {
	ctx->width  = width;
	ctx->height = height;
	ctx->meta   = NULL;

	/* Allocates a power-of-2 sized bitmap equal to or greater than the given size, and clears it to 0 */
	width  = Math_NextPowOf2(width);
	height = Math_NextPowOf2(height);

	ctx->bmp.width  = width; 
	ctx->bmp.height = height;
	ctx->bmp.scan0  = (BitmapCol*)Mem_AllocCleared(width * height, 4, "bitmap data");
}

void Context2D_Wrap(struct Context2D* ctx, struct Bitmap* bmp) {
	ctx->bmp    = *bmp;
	ctx->width  = bmp->width;
	ctx->height = bmp->height;
	ctx->meta   = NULL;
}

void Context2D_Free(struct Context2D* ctx) {
	Mem_Free(ctx->bmp.scan0);
}

void Gradient_Noise(struct Context2D* ctx, BitmapCol color, int variation,
					int x, int y, int width, int height) {
	struct Bitmap* bmp = (struct Bitmap*)ctx;
	BitmapCol* dst;
	int R, G, B, xx, yy, n;
	float noise;
	if (!Drawer2D_Clamp(ctx, &x, &y, &width, &height)) return;

	for (yy = 0; yy < height; yy++) {
		dst = Bitmap_GetRow(bmp, y + yy) + x;

		for (xx = 0; xx < width; xx++, dst++) {
			n = (x + xx) + (y + yy) * 57;
			n = (n << 13) ^ n;
			noise = 1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f;

			R = BitmapCol_R(color) + (int)(noise * variation); Drawer2D_ClampPixel(R);
			G = BitmapCol_G(color) + (int)(noise * variation); Drawer2D_ClampPixel(G);
			B = BitmapCol_B(color) + (int)(noise * variation); Drawer2D_ClampPixel(B);

			*dst = BitmapColor_RGB(R, G, B);
		}
	}
}

void Gradient_Vertical(struct Context2D* ctx, BitmapCol a, BitmapCol b,
					   int x, int y, int width, int height) {
	struct Bitmap* bmp = (struct Bitmap*)ctx;
	BitmapCol* row, color;
	int xx, yy;
	float t;
	if (!Drawer2D_Clamp(ctx, &x, &y, &width, &height)) return;

	for (yy = 0; yy < height; yy++) {
		row = Bitmap_GetRow(bmp, y + yy) + x;
		t   = (float)yy / (height - 1); /* so last row has color of b */

		color = BitmapCol_Make(
			Math_Lerp(BitmapCol_R(a), BitmapCol_R(b), t),
			Math_Lerp(BitmapCol_G(a), BitmapCol_G(b), t),
			Math_Lerp(BitmapCol_B(a), BitmapCol_B(b), t),
			255);

		for (xx = 0; xx < width; xx++) { row[xx] = color; }
	}
}

void Gradient_Blend(struct Context2D* ctx, BitmapCol color, int blend,
					int x, int y, int width, int height) {
	struct Bitmap* bmp = (struct Bitmap*)ctx;
	BitmapCol* dst;
	int R, G, B, xx, yy;
	if (!Drawer2D_Clamp(ctx, &x, &y, &width, &height)) return;

	/* Pre compute the alpha blended source color */
	/* TODO: Avoid shift when multiplying */
	color = BitmapCol_Make(
		BitmapCol_R(color) * blend / 255,
		BitmapCol_G(color) * blend / 255,
		BitmapCol_B(color) * blend / 255,
		0);
	blend = 255 - blend; /* inverse for existing pixels */

	for (yy = 0; yy < height; yy++) {
		dst = Bitmap_GetRow(bmp, y + yy) + x;

		for (xx = 0; xx < width; xx++, dst++) {
			/* TODO: Not shift when multiplying */
			R = BitmapCol_R(color) + (BitmapCol_R(*dst) * blend) / 255;
			G = BitmapCol_G(color) + (BitmapCol_G(*dst) * blend) / 255;
			B = BitmapCol_B(color) + (BitmapCol_B(*dst) * blend) / 255;

			*dst = BitmapColor_RGB(R, G, B);
		}
	}
}

void Context2D_DrawPixels(struct Context2D* ctx, int x, int y, struct Bitmap* src) {
	struct Bitmap* dst = (struct Bitmap*)ctx;
	int width = src->width, height = src->height;
	BitmapCol* dstRow;
	BitmapCol* srcRow;
	int xx, yy;
	if (!Drawer2D_Clamp(ctx, &x, &y, &width, &height)) return;

	for (yy = 0; yy < height; yy++) {
		srcRow = Bitmap_GetRow(src, yy);
		dstRow = Bitmap_GetRow(dst, y + yy) + x;

		for (xx = 0; xx < width; xx++) { dstRow[xx] = srcRow[xx]; }
	}
}

void Context2D_Clear(struct Context2D* ctx, BitmapCol color,
					int x, int y, int width, int height) {
	struct Bitmap* bmp = (struct Bitmap*)ctx;
	BitmapCol* row;
	int xx, yy;
	if (!Drawer2D_Clamp(ctx, &x, &y, &width, &height)) return;

	for (yy = 0; yy < height; yy++) {
		row = Bitmap_GetRow(bmp, y + yy) + x;
		for (xx = 0; xx < width; xx++) { row[xx] = color; }
	}
}


void Drawer2D_MakeTextTexture(struct Texture* tex, struct DrawTextArgs* args) {
	static struct Texture empty = { 0, Tex_Rect(0,0, 0,0), Tex_UV(0,0, 1,1) };
	struct Context2D ctx;
	int width, height;
	/* pointless to draw anything when context is lost */
	if (Gfx.LostContext) { *tex = empty; return; }

	width  = Drawer2D_TextWidth(args);
	if (!width) { *tex = empty; return; }
	height = Drawer2D_TextHeight(args);

	Context2D_Alloc(&ctx, width, height);
	{
		Context2D_DrawText(&ctx, args, 0, 0);
		Context2D_MakeTexture(tex, &ctx);
	}
	Context2D_Free(&ctx);
}

void Context2D_MakeTexture(struct Texture* tex, struct Context2D* ctx) {
	Gfx_RecreateTexture(&tex->ID, &ctx->bmp, 0, false);
	tex->Width  = ctx->width;
	tex->Height = ctx->height;

	tex->uv.U1 = 0.0f; tex->uv.V1 = 0.0f;
	tex->uv.U2 = (float)ctx->width  / (float)ctx->bmp.width;
	tex->uv.V2 = (float)ctx->height / (float)ctx->bmp.height;
}

cc_bool Drawer2D_ValidColorCodeAt(const cc_string* text, int i) {
	if (i >= text->length) return false;
	return BitmapCol_A(Drawer2D_GetColor(text->buffer[i])) != 0;
}

cc_bool Drawer2D_UNSAFE_NextPart(cc_string* left, cc_string* part, char* colorCode) {
	BitmapCol color;
	char cur;
	int i;

	/* check if current part starts with a colour code */
	if (left->length >= 2 && left->buffer[0] == '&') {
		cur   = left->buffer[1];
		color = Drawer2D_GetColor(cur);
		
		if (BitmapCol_A(color)) {
			*colorCode = cur;
			left->buffer += 2;
			left->length -= 2;
		}
	}

	for (i = 0; i < left->length; i++) 
	{
		if (left->buffer[i] == '&' && Drawer2D_ValidColorCodeAt(left, i + 1)) break;
	}

	/* advance string starts and lengths */
	part->buffer  = left->buffer;
	part->length  = i;
	left->buffer += i;
	left->length -= i;

	return part->length > 0 || left->length > 0;
}

cc_bool Drawer2D_IsEmptyText(const cc_string* text) {
	cc_string left = *text, part;
	char colorCode;

	while (Drawer2D_UNSAFE_NextPart(&left, &part, &colorCode))
	{
		if (part.length) return false;
	}
	return true;
}

void Drawer2D_WithoutColors(cc_string* str, const cc_string* src) {
	cc_string left = *src, part;
	char colorCode;

	while (Drawer2D_UNSAFE_NextPart(&left, &part, &colorCode))
	{
		String_AppendString(str, &part);
	}
}

char Drawer2D_LastColor(const cc_string* text, int start) {
	int i;
	if (start >= text->length) start = text->length - 1;
	
	for (i = start; i >= 0; i--) {
		if (text->buffer[i] != '&') continue;
		if (Drawer2D_ValidColorCodeAt(text, i + 1)) {
			return text->buffer[i + 1];
		}
	}
	return '\0';
}
cc_bool Drawer2D_IsWhiteColor(char c) { return c == '\0' || c == 'f' || c == 'F'; }

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

void Drawer2D_Fill(struct Bitmap* bmp, int x, int y, int width, int height, BitmapCol color) {
	BitmapCol* row;
	int xx, yy;

	for (yy = y; yy < y + height; yy++) {
		if (yy >= bmp->height) return;
		row = Bitmap_GetRow(bmp, yy);

		for (xx = x; xx < x + width; xx++) {
			if (xx >= bmp->width) break;
			row[xx] = color;
		}
	}
}

static void DrawBitmappedTextCore(struct Bitmap* bmp, struct DrawTextArgs* args, int x, int y, cc_bool shadow) {
	BitmapCol color;
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

	cc_uint8 coords[DRAWER2D_MAX_TEXT_LENGTH];
	BitmapCol colors[DRAWER2D_MAX_TEXT_LENGTH];
	cc_uint16 dstWidths[DRAWER2D_MAX_TEXT_LENGTH];

	color = Drawer2D.Colors['f'];
	if (shadow) color = GetShadowColor(color);

	for (i = 0; i < text.length; i++) {
		char c = text.buffer[i];
		if (c == '&' && Drawer2D_ValidColorCodeAt(&text, i + 1)) {
			color = Drawer2D_GetColor(text.buffer[i + 1]);

			if (shadow) color = GetShadowColor(color);
			i++; continue; /* skip over the color code */
		}

		coords[count] = c;
		colors[count] = color;
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
			color    = colors[i];

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
					BitmapCol_R(src) * BitmapCol_R(color) / 255,
					BitmapCol_G(src) * BitmapCol_G(color) / 255,
					BitmapCol_B(src) * BitmapCol_B(color) / 255,
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
		color    = colors[i];

		for (; i < count && color == colors[i]; i++) {
			dstWidth += dstWidths[i] + xPadding;
		}
		Drawer2D_Fill(bmp, x, underlineY, dstWidth, underlineHeight, color);
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
		if (c == '&' && Drawer2D_ValidColorCodeAt(&text, i + 1)) {
			i++; continue; /* skip over the color code */
		}
		width += Drawer2D_Width(point, c) + xPadding;
	}
	if (!width) return 0;

	/* Remove padding at end */
	if (!(args->font->flags & FONT_FLAGS_PADDING)) width -= xPadding;

	if (args->useShadow) { width += Drawer2D_ShadowOffset(point); }
	return width;
}

void Context2D_DrawText(struct Context2D* ctx, struct DrawTextArgs* args, int x, int y) {
	struct Bitmap* bmp = (struct Bitmap*)ctx;
	if (Drawer2D_IsEmptyText(&args->text)) return;
	if (Font_IsBitmap(args->font)) { DrawBitmappedText(bmp, args, x, y); return; }

	if (args->useShadow) { SysFont_DrawText(args, bmp, x, y, true); }
	SysFont_DrawText(args, bmp, x, y, false);
}

int Drawer2D_TextWidth(struct DrawTextArgs* args) {
	if (Font_IsBitmap(args->font)) return MeasureBitmappedWidth(args);
	return SysFont_TextWidth(args);
}

int Drawer2D_TextHeight(struct DrawTextArgs* args) {
	return Font_CalcHeight(args->font, args->useShadow);
}

int Font_CalcHeight(const struct FontDesc* font, cc_bool useShadow) {
	int height = font->height;
	if (Font_IsBitmap(font)) {
		if (useShadow) { height += Drawer2D_ShadowOffset(font->size); }
	} else {
		if (useShadow) height += 2;
	}
	return height;
}

void Drawer2D_DrawClippedText(struct Context2D* ctx, struct DrawTextArgs* args,
								int x, int y, int maxWidth) {
	char strBuffer[512];
	struct DrawTextArgs part;
	int i, width;

	width = Drawer2D_TextWidth(args);
	/* No clipping needed */
	if (width <= maxWidth) { Context2D_DrawText(ctx, args, x, y); return; }
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
		if (width <= maxWidth) { Context2D_DrawText(ctx, &part, x, y); return; }

		/* If down to <= 2 chars, try omitting the .. */
		if (i > 2) continue;
		part.text.length = i;
		width            = Drawer2D_TextWidth(&part);
		if (width <= maxWidth) { Context2D_DrawText(ctx, &part, x, y); return; }
	}
}


/*########################################################################################################################*
*---------------------------------------------------Drawer2D component----------------------------------------------------*
*#########################################################################################################################*/
static void DefaultPngProcess(struct Stream* stream, const cc_string* name) {
	struct Bitmap bmp;
	cc_result res;

	if ((res = Png_Decode(&bmp, stream))) {
		Logger_SysWarn2(res, "decoding", name);
		Mem_Free(bmp.scan0);
	} else if (Font_SetBitmapAtlas(&bmp)) {
		Event_RaiseVoid(&ChatEvents.FontChanged);
	} else {
		Mem_Free(bmp.scan0);
	}
}
static struct TextureEntry default_entry = { "default.png", DefaultPngProcess };


static void InitHexEncodedColor(int i, int hex, cc_uint8 lo, cc_uint8 hi) {
	Drawer2D.Colors[i] = BitmapColor_RGB(
		lo * ((hex >> 2) & 1) + hi * (hex >> 3),
		lo * ((hex >> 1) & 1) + hi * (hex >> 3),
		lo * ((hex >> 0) & 1) + hi * (hex >> 3));
}

static void OnReset(void) {
	int i;	
	for (i = 0; i < DRAWER2D_MAX_COLORS; i++) {
		Drawer2D.Colors[i] = 0;
	}

	for (i = 0; i <= 9; i++) {
		InitHexEncodedColor('0' + i, i, 191, 64);
	}
	for (i = 10; i <= 15; i++) {
		InitHexEncodedColor('a' + (i - 10), i, 191, 64);
		InitHexEncodedColor('A' + (i - 10), i, 191, 64);
	}
}

static void OnInit(void) {
	OnReset();
	TextureEntry_Register(&default_entry);

	Drawer2D.BitmappedText    = Game_ClassicMode || !Options_GetBool(OPT_USE_CHAT_FONT, false);
	Drawer2D.BlackTextShadows = Options_GetBool(OPT_BLACK_TEXT, false);
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