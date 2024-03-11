#include "SystemFonts.h"
#include "Drawer2D.h"
#include "String.h"
#include "Funcs.h"
#include "Platform.h"
#include "ExtMath.h"
#include "Logger.h"
#include "Game.h"
#include "Event.h"
#include "Stream.h"
#include "Utils.h"
#include "Errors.h"
#include "Window.h"
#include "Options.h"

static char defaultBuffer[STRING_SIZE];
static cc_string font_default = String_FromArray(defaultBuffer);

void SysFont_SetDefault(const cc_string* fontName) {
	String_Copy(&font_default, fontName);
	Event_RaiseVoid(&ChatEvents.FontChanged);
}

static void OnInit(void) {
	Options_Get(OPT_FONT_NAME, &font_default, "");
	if (Game_ClassicMode) font_default.length = 0;
}

struct IGameComponent SystemFonts_Component = {
	OnInit /* Init  */
};


/*########################################################################################################################*
*--------------------------------------------------------Freetype---------------------------------------------------------*
*#########################################################################################################################*/
#if defined CC_BUILD_FREETYPE
#include "freetype/ft2build.h"
#include "freetype/freetype.h"
#include "freetype/ftmodapi.h"
#include "freetype/ftglyph.h"

static FT_Library ft_lib;
static struct FT_MemoryRec_ ft_mem;
static struct StringsBuffer font_list;
static cc_bool fonts_changed;
/* Finds the path and face number of the given system font, with closest matching style */
static cc_string Font_Lookup(const cc_string* fontName, int flags);

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

static void SysFont_Done(struct SysFont* font) {
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
	SysFont_Done(font);
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
static cc_string font_candidates[] = {
	String_FromConst(""),                /* replaced with font_default */
	String_FromConst("Arial"),           /* preferred font on all platforms */
	String_FromConst("Liberation Sans"), /* Nice looking fallbacks for linux */
	String_FromConst("Nimbus Sans"),
	String_FromConst("Bitstream Charter"),
	String_FromConst("Cantarell"),
	String_FromConst("DejaVu Sans Book"), 
	String_FromConst("Century Schoolbook L Roman"), /* commonly available on linux */
	String_FromConst("Liberation Serif"), /* for SerenityOS */
	String_FromConst("Slate For OnePlus"), /* Android 10, some devices */
	String_FromConst("Roboto"), /* Android (broken on some Android 10 devices) */
	String_FromConst("Geneva"), /* for ancient macOS versions */
	String_FromConst("Droid Sans"), /* for old Android versions */
	String_FromConst("Google Sans") /* Droid Sans is now known as Google Sans on some Android devices (e.g. a Pixel 6) */
};

static void InitFreeTypeLibrary(void) {
	FT_Error err;
	if (ft_lib) return;

	ft_mem.alloc   = FT_AllocWrapper;
	ft_mem.free    = FT_FreeWrapper;
	ft_mem.realloc = FT_ReallocWrapper;

	err = FT_New_Library(&ft_mem, &ft_lib);
	if (err) Logger_Abort2(err, "Failed to init freetype");
	FT_Add_Default_Modules(ft_lib);
}

static cc_bool loadedPlatformFonts;
/* Updates fonts list cache with platform's list of fonts */
/* This should be avoided due to overhead potential */
static void SysFonts_LoadPlatform(void) {
	if (loadedPlatformFonts) return;
	loadedPlatformFonts = true;

	/* TODO this basically gets called all the time on non-Window platforms */
	/* Maybe we can cache the default system font to avoid this extra work? */
	if (font_list.count == 0)
		Window_ShowDialog("One time load", "Initialising font cache, this can take several seconds.");

	InitFreeTypeLibrary();
	Platform_LoadSysFonts();

	if (fonts_changed) EntryList_Save(&font_list, FONT_CACHE_FILE);
}

static cc_bool loadedCachedFonts;
static void SysFonts_LoadCached(void) {
	if (loadedCachedFonts) return;
	loadedCachedFonts = true;

	EntryList_UNSAFE_Load(&font_list, FONT_CACHE_FILE);
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
	if (err) { SysFont_Done(&font); return 0; }

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


static cc_string Font_LookupOf(const cc_string* fontName, const char type) {
	cc_string name; char nameBuffer[STRING_SIZE + 2];
	String_InitArray(name, nameBuffer);

	String_Format2(&name, "%s %r", fontName, &type);
	return EntryList_UNSAFE_Get(&font_list, &name, '=');
}

static cc_string Font_DoLookup(const cc_string* fontName, int flags) {
	cc_string path = String_Empty;

	if (flags & FONT_FLAGS_BOLD) path = Font_LookupOf(fontName, 'B');
	return path.length ? path : Font_LookupOf(fontName, 'R');
}

static cc_string Font_Lookup(const cc_string* fontName, int flags) {
	cc_string path;
	
	SysFonts_LoadCached();
	path = Font_DoLookup(fontName, flags);
	if (path.length) return path;

	SysFonts_LoadPlatform();
	return Font_DoLookup(fontName, flags);
}


const cc_string* SysFonts_UNSAFE_GetDefault(void) {
	cc_string* font, path;
	int i;
	font_candidates[0] = font_default;

	for (i = 0; i < Array_Elems(font_candidates); i++) {
		font = &font_candidates[i];
		if (!font->length) continue;

		path = Font_Lookup(font, FONT_FLAGS_NONE);
		if (path.length) return font;
	}
	return &String_Empty;
}

void SysFonts_GetNames(struct StringsBuffer* buffer) {
	cc_string entry, name, path;
	int i;
	SysFonts_LoadCached();
	SysFonts_LoadPlatform();

	for (i = 0; i < font_list.count; i++) {
		StringsBuffer_UNSAFE_GetRaw(&font_list, i, &entry);
		String_UNSAFE_Separate(&entry, '=', &name, &path);

		/* only want Regular fonts here */
		if (name.length < 2 || name.buffer[name.length - 1] != 'R') continue;
		name.length -= 2;
		StringsBuffer_Add(buffer, &name);
	}
	StringsBuffer_Sort(buffer);
}

#define TEXT_CEIL(x) (((x) + 63) >> 6)
cc_result SysFont_Make(struct FontDesc* desc, const cc_string* fontName, int size, int flags) {
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

	InitFreeTypeLibrary();
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

void SysFont_MakeDefault(struct FontDesc* desc, int size, int flags) {
	cc_string* font;
	cc_result res;
	int i;
	font_candidates[0] = font_default;

	for (i = 0; i < Array_Elems(font_candidates); i++) {
		font = &font_candidates[i];
		if (!font->length) continue;
		res  = SysFont_Make(desc, &font_candidates[i], size, flags);

		/* Cached system fonts list may be outdated - force update it */
		if (res == ReturnCode_FileNotFound && !loadedPlatformFonts) {
			StringsBuffer_Clear(&font_list);
			SysFonts_LoadPlatform();
			res = SysFont_Make(desc, &font_candidates[i], size, flags);
		}

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

void SysFont_Free(struct FontDesc* desc) {
	struct SysFont* font = (struct SysFont*)desc->handle;
	FT_Done_Face(font->face);
	Mem_Free(font);
}

int SysFont_TextWidth(struct DrawTextArgs* args) {
	struct SysFont* font = (struct SysFont*)args->font->handle;
	FT_Face face   = font->face;
	cc_string text = args->text;
	int i, width = 0, charWidth;
	FT_Error res;
	cc_unichar uc;

	for (i = 0; i < text.length; i++) {
		char c = text.buffer[i];
		if (c == '&' && Drawer2D_ValidColorCodeAt(&text, i + 1)) {
			i++; continue; /* skip over the color code */
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
	if (!width) return 0;

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
				*dst = col | BitmapColor_A_Bits(255);
			}
		}
	}
}

static FT_Vector shadow_delta = { 83, -83 };
void SysFont_DrawText(struct DrawTextArgs* args, struct Bitmap* bmp, int x, int y, cc_bool shadow) {
	struct SysFont* font  = (struct SysFont*)args->font->handle;
	FT_BitmapGlyph* glyphs = font->glyphs;

	FT_Face face   = font->face;
	cc_string text = args->text;
	int descender, height, begX = x;
	BitmapCol color;
	
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

	color = Drawer2D.Colors['f'];
	if (shadow) color = GetShadowColor(color);

	for (i = 0; i < text.length; i++) {
		char c = text.buffer[i];
		if (c == '&' && Drawer2D_ValidColorCodeAt(&text, i + 1)) {
			color = Drawer2D_GetColor(text.buffer[i + 1]);

			if (shadow) color = GetShadowColor(color);
			i++; continue; /* skip over the color code */
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
			DrawBlackWhiteGlyph(img, bmp, x, y, color);
		} else {
			DrawGrayscaleGlyph(img, bmp, x, y, color);
		}

		x += TEXT_CEIL(glyph->root.advance.x >> 10);
		x -= glyph->left; y -= offset;
	}

	if (args->font->flags & FONT_FLAGS_UNDERLINE) {
		int ul_pos   = FT_MulFix(face->underline_position,  face->size->metrics.y_scale);
		int ul_thick = FT_MulFix(face->underline_thickness, face->size->metrics.y_scale);

		int ulHeight = TEXT_CEIL(ul_thick);
		int ulY      = height + TEXT_CEIL(ul_pos);
		Drawer2D_Fill(bmp, begX, ulY + y, x - begX, ulHeight, color);
	}

	if (shadow) FT_Set_Transform(face, NULL, NULL);
}
#elif defined CC_BUILD_WEB
static cc_string font_arial = String_FromConst("Arial");

const cc_string* SysFonts_UNSAFE_GetDefault(void) {
	/* Fallback to Arial as default font */
	/* TODO use serif instead?? */
	return font_default.length ? &font_default : &font_arial;
}

void SysFonts_GetNames(struct StringsBuffer* buffer) {
	static const char* font_names[] = { 
		"Arial", "Arial Black", "Courier New", "Comic Sans MS", "Georgia", "Garamond", 
		"Helvetica", "Impact", "Tahoma", "Times New Roman", "Trebuchet MS", "Verdana",
		"cursive", "fantasy", "monospace", "sans-serif", "serif", "system-ui"
	};
	int i;

	for (i = 0; i < Array_Elems(font_names); i++) {
		cc_string str = String_FromReadonly(font_names[i]);
		StringsBuffer_Add(buffer, &str);
	}
}

cc_result SysFont_Make(struct FontDesc* desc, const cc_string* fontName, int size, int flags) {
	desc->size   = size;
	desc->flags  = flags;
	desc->height = Drawer2D_AdjHeight(size);

	desc->handle = Mem_TryAlloc(fontName->length + 1, 1);
	if (!desc->handle) return ERR_OUT_OF_MEMORY;
	
	String_CopyToRaw(desc->handle, fontName->length + 1, fontName);
	return 0;
}

void SysFont_MakeDefault(struct FontDesc* desc, int size, int flags) {
	SysFont_Make(desc, SysFonts_UNSAFE_GetDefault(), size, flags);
}

void SysFont_Free(struct FontDesc* desc) {
	Mem_Free(desc->handle);
}

void SysFonts_Register(const cc_string* path) { }
extern void   interop_SetFont(const char* font, int size, int flags);
extern double interop_TextWidth(const char* text, const int len);
extern double interop_TextDraw(const char* text, const int len, struct Bitmap* bmp, int x, int y, cc_bool shadow, const char* hex);

int SysFont_TextWidth(struct DrawTextArgs* args) {
	struct FontDesc* font = args->font;
	cc_string left = args->text, part;
	double width   = 0;
	char colorCode;

	interop_SetFont(font->handle, font->size, font->flags);
	while (Drawer2D_UNSAFE_NextPart(&left, &part, &colorCode))
	{
		char buffer[NATIVE_STR_LEN];
		int len = String_EncodeUtf8(buffer, &part);
		width += interop_TextWidth(buffer, len);
	}
	return Math_Ceil(width);
}

void SysFont_DrawText(struct DrawTextArgs* args, struct Bitmap* bmp, int x, int y, cc_bool shadow) {
	struct FontDesc* font = args->font;
	cc_string left  = args->text, part;
	BitmapCol color;
	char colorCode = 'f';
	double xOffset = 0;
	char hexBuffer[7];
	cc_string hex;

	/* adjust y position to more closely match FreeType drawn text */
	y += (args->font->height - args->font->size) / 2;
	interop_SetFont(font->handle, font->size, font->flags);

	while (Drawer2D_UNSAFE_NextPart(&left, &part, &colorCode))
	{
		char buffer[NATIVE_STR_LEN];
		int len = String_EncodeUtf8(buffer, &part);

		color = Drawer2D_GetColor(colorCode);
		if (shadow) color = GetShadowColor(color);

		String_InitArray(hex, hexBuffer);
		String_Append(&hex, '#');
		String_AppendHex(&hex, BitmapCol_R(color));
		String_AppendHex(&hex, BitmapCol_G(color));
		String_AppendHex(&hex, BitmapCol_B(color));

		/* TODO pass as double directly instead of (int) ?*/
		xOffset += interop_TextDraw(buffer, len, bmp, x + (int)xOffset, y, shadow, hexBuffer);
	}
}
#elif defined CC_BUILD_IOS
/* implemented in interop_ios.m */
extern void interop_GetFontNames(struct StringsBuffer* buffer);
extern cc_result interop_SysFontMake(struct FontDesc* desc, const cc_string* fontName, int size, int flags);
extern void interop_SysMakeDefault(struct FontDesc* desc, int size, int flags);
extern void interop_SysFontFree(void* handle);
extern int interop_SysTextWidth(struct DrawTextArgs* args);
extern void interop_SysTextDraw(struct DrawTextArgs* args, struct Bitmap* bmp, int x, int y, cc_bool shadow);

void SysFonts_Register(const cc_string* path) { }

const cc_string* SysFonts_UNSAFE_GetDefault(void) {
    return &String_Empty;
}

void SysFonts_GetNames(struct StringsBuffer* buffer) {
    interop_GetFontNames(buffer);
}

cc_result SysFont_Make(struct FontDesc* desc, const cc_string* fontName, int size, int flags) {
    return interop_SysFontMake(desc, fontName, size, flags);
}

void SysFont_MakeDefault(struct FontDesc* desc, int size, int flags) {
    interop_SysMakeDefault(desc, size, flags);
}

void Font_Free(struct FontDesc* desc) {
    interop_SysFontFree(desc->handle);
}

int SysFont_TextWidth(struct DrawTextArgs* args) {
    return interop_SysTextWidth(args);
}

void SysFont_DrawText(struct DrawTextArgs* args, struct Bitmap* bmp, int x, int y, cc_bool shadow) {
    interop_SysTextDraw(args, bmp, x, y, shadow);
}
#else

#define SysFont_ValidChar(c) ((c) > 32 && (c) < 127)
#define SysFont_ToIndex(c)   ((c) - 33) /* First valid char is ! */
#define SPACE_WIDTH 2
#define CELL_SIZE 8

#define SysFont_GetRows(c) (SysFont_ValidChar(c) ? font_bitmap[SysFont_ToIndex(c)] : missing_cell)

static cc_uint8 missing_cell[CELL_SIZE] = { 
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF 
};

/* 8x8 font bitmap, represented with 1 bit for each pixel */
/* Source: Goodly's texture pack for ClassiCube */
static cc_uint8 font_bitmap[][CELL_SIZE] = {
	  { 0x01,0x01,0x01,0x01,0x01,0x00,0x01,0x00 }, /* ! */
	  { 0x05,0x05,0x05,0x00,0x00,0x00,0x00,0x00 }, /* " */
	  { 0x0A,0x0A,0x1F,0x0A,0x1F,0x0A,0x0A,0x00 }, /* # */
	  { 0x04,0x1F,0x01,0x1F,0x10,0x1F,0x04,0x00 }, /* $ */
	  { 0x00,0x21,0x11,0x08,0x04,0x22,0x21,0x00 }, /* % */
	  { 0x0C,0x12,0x0C,0x2E,0x19,0x11,0x2E,0x00 }, /* & */
	  { 0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00 }, /* ' */
	  { 0x04,0x02,0x01,0x01,0x01,0x02,0x04,0x00 }, /* ( */
	  { 0x01,0x02,0x04,0x04,0x04,0x02,0x01,0x00 }, /* ) */
	  { 0x00,0x02,0x07,0x02,0x05,0x00,0x00,0x00 }, /* * */
	  { 0x00,0x04,0x04,0x1F,0x04,0x04,0x00,0x00 }, /* + */
	  { 0x00,0x00,0x00,0x00,0x00,0x02,0x02,0x01 }, /* , */
	  { 0x00,0x00,0x00,0x1F,0x00,0x00,0x00,0x00 }, /* - */
	  { 0x00,0x00,0x00,0x00,0x00,0x01,0x01,0x00 }, /* . */
	  { 0x08,0x08,0x04,0x04,0x02,0x02,0x01,0x00 }, /* / */
	  { 0x06,0x09,0x0D,0x0B,0x09,0x09,0x06,0x00 }, /* 0 */
	  { 0x02,0x03,0x02,0x02,0x02,0x02,0x07,0x00 }, /* 1 */
	  { 0x06,0x09,0x08,0x04,0x02,0x09,0x0F,0x00 }, /* 2 */
	  { 0x06,0x09,0x08,0x06,0x08,0x09,0x06,0x00 }, /* 3 */
	  { 0x05,0x05,0x05,0x0F,0x04,0x04,0x04,0x00 }, /* 4 */
	  { 0x0F,0x01,0x07,0x08,0x08,0x09,0x06,0x00 }, /* 5 */
	  { 0x06,0x09,0x01,0x07,0x09,0x09,0x06,0x00 }, /* 6 */
	  { 0x0F,0x08,0x08,0x04,0x04,0x02,0x02,0x00 }, /* 7 */
	  { 0x06,0x09,0x09,0x06,0x09,0x09,0x06,0x00 }, /* 8 */
	  { 0x06,0x09,0x09,0x0E,0x08,0x09,0x06,0x00 }, /* 9 */
	  { 0x00,0x01,0x01,0x00,0x00,0x01,0x01,0x00 }, /* : */
	  { 0x00,0x02,0x02,0x00,0x00,0x02,0x02,0x01 }, /* ; */
	  { 0x00,0x04,0x02,0x01,0x02,0x04,0x00,0x00 }, /* < */
	  { 0x00,0x00,0x1F,0x00,0x00,0x1F,0x00,0x00 }, /* = */
	  { 0x00,0x01,0x02,0x04,0x02,0x01,0x00,0x00 }, /* > */
	  { 0x07,0x09,0x08,0x04,0x02,0x00,0x02,0x00 }, /* ? */
	  { 0x0E,0x11,0x1D,0x1D,0x1D,0x01,0x0E,0x00 }, /* @ */
	  { 0x06,0x09,0x09,0x0F,0x09,0x09,0x09,0x00 }, /* A */
	  { 0x07,0x09,0x09,0x07,0x09,0x09,0x07,0x00 }, /* B */
	  { 0x06,0x09,0x01,0x01,0x01,0x09,0x06,0x00 }, /* C */
	  { 0x07,0x09,0x09,0x09,0x09,0x09,0x07,0x00 }, /* D */
	  { 0x0F,0x01,0x01,0x07,0x01,0x01,0x0F,0x00 }, /* E */
	  { 0x0F,0x01,0x01,0x07,0x01,0x01,0x01,0x00 }, /* F */
	  { 0x06,0x09,0x01,0x0D,0x09,0x09,0x06,0x00 }, /* G */
	  { 0x09,0x09,0x09,0x0F,0x09,0x09,0x09,0x00 }, /* H */
	  { 0x07,0x02,0x02,0x02,0x02,0x02,0x07,0x00 }, /* I */
	  { 0x08,0x08,0x08,0x08,0x08,0x09,0x07,0x00 }, /* J */
	  { 0x09,0x09,0x05,0x03,0x05,0x09,0x09,0x00 }, /* K */
	  { 0x01,0x01,0x01,0x01,0x01,0x01,0x0F,0x00 }, /* L */
	  { 0x11,0x1B,0x15,0x11,0x11,0x11,0x11,0x00 }, /* M */
	  { 0x09,0x0B,0x0D,0x09,0x09,0x09,0x09,0x00 }, /* N */
	  { 0x06,0x09,0x09,0x09,0x09,0x09,0x06,0x00 }, /* O */
	  { 0x07,0x09,0x09,0x07,0x01,0x01,0x01,0x00 }, /* P */
	  { 0x06,0x09,0x09,0x09,0x09,0x05,0x0E,0x00 }, /* Q */
	  { 0x07,0x09,0x09,0x07,0x09,0x09,0x09,0x00 }, /* R */
	  { 0x06,0x09,0x01,0x06,0x08,0x09,0x06,0x00 }, /* S */
	  { 0x07,0x02,0x02,0x02,0x02,0x02,0x02,0x00 }, /* T */
	  { 0x09,0x09,0x09,0x09,0x09,0x09,0x06,0x00 }, /* U */
	  { 0x11,0x11,0x11,0x11,0x11,0x0A,0x04,0x00 }, /* V */
	  { 0x11,0x11,0x11,0x11,0x15,0x1B,0x11,0x00 }, /* W */
	  { 0x11,0x11,0x0A,0x04,0x0A,0x11,0x11,0x00 }, /* X */
	  { 0x11,0x11,0x0A,0x04,0x04,0x04,0x04,0x00 }, /* Y */
	  { 0x0F,0x08,0x04,0x02,0x01,0x01,0x0F,0x00 }, /* Z */
	  { 0x07,0x01,0x01,0x01,0x01,0x01,0x07,0x00 }, /* [ */
	  { 0x01,0x01,0x02,0x02,0x04,0x04,0x08,0x00 }, /* \ */
	  { 0x07,0x04,0x04,0x04,0x04,0x04,0x07,0x00 }, /* ] */
	  { 0x04,0x0A,0x11,0x00,0x00,0x00,0x00,0x00 }, /* ^ */
	  { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1F }, /* _ */
	  { 0x01,0x01,0x02,0x00,0x00,0x00,0x00,0x00 }, /* ` */
	  { 0x00,0x00,0x0E,0x09,0x09,0x0D,0x0B,0x00 }, /* a */
	  { 0x01,0x01,0x07,0x09,0x09,0x09,0x07,0x00 }, /* b */
	  { 0x00,0x00,0x06,0x09,0x01,0x09,0x06,0x00 }, /* c */
	  { 0x08,0x08,0x0E,0x09,0x09,0x09,0x0E,0x00 }, /* d */
	  { 0x00,0x00,0x06,0x09,0x0F,0x01,0x0E,0x00 }, /* e */
	  { 0x06,0x01,0x07,0x01,0x01,0x01,0x01,0x00 }, /* f */
	  { 0x00,0x00,0x0E,0x09,0x09,0x0E,0x08,0x07 }, /* g */
	  { 0x01,0x01,0x07,0x09,0x09,0x09,0x09,0x00 }, /* h */
	  { 0x01,0x00,0x01,0x01,0x01,0x01,0x01,0x00 }, /* i */
	  { 0x08,0x00,0x08,0x08,0x08,0x08,0x09,0x06 }, /* j */
	  { 0x01,0x01,0x09,0x05,0x03,0x05,0x09,0x00 }, /* k */
	  { 0x01,0x01,0x01,0x01,0x01,0x01,0x02,0x00 }, /* l */
	  { 0x00,0x00,0x0B,0x15,0x15,0x11,0x11,0x00 }, /* m */
	  { 0x00,0x00,0x07,0x09,0x09,0x09,0x09,0x00 }, /* n */
	  { 0x00,0x00,0x06,0x09,0x09,0x09,0x06,0x00 }, /* o */
	  { 0x00,0x00,0x07,0x09,0x09,0x07,0x01,0x01 }, /* p */
	  { 0x00,0x00,0x0E,0x09,0x09,0x0E,0x08,0x08 }, /* q */
	  { 0x00,0x00,0x05,0x03,0x01,0x01,0x01,0x00 }, /* r */
	  { 0x00,0x00,0x0E,0x01,0x06,0x08,0x07,0x00 }, /* s */
	  { 0x02,0x02,0x07,0x02,0x02,0x02,0x02,0x00 }, /* t */
	  { 0x00,0x00,0x09,0x09,0x09,0x09,0x0E,0x00 }, /* u */
	  { 0x00,0x00,0x09,0x09,0x09,0x05,0x03,0x00 }, /* v */
	  { 0x00,0x00,0x11,0x11,0x15,0x15,0x1A,0x00 }, /* w */
	  { 0x00,0x00,0x05,0x05,0x02,0x05,0x05,0x00 }, /* x */
	  { 0x00,0x00,0x09,0x09,0x09,0x0E,0x08,0x07 }, /* y */
	  { 0x00,0x00,0x0F,0x08,0x04,0x02,0x0F,0x00 }, /* z */
	  { 0x04,0x02,0x02,0x01,0x02,0x02,0x04,0x00 }, /* { */
	  { 0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01 }, /* | */
	  { 0x01,0x02,0x02,0x04,0x02,0x02,0x01,0x00 }, /* } */
	  { 0x00,0x00,0x26,0x19,0x00,0x00,0x00,0x00 }, /* ~ */
};


void SysFonts_Register(const cc_string* path) { }

const cc_string* SysFonts_UNSAFE_GetDefault(void) { return &String_Empty; }

void SysFonts_GetNames(struct StringsBuffer* buffer) { }

cc_result SysFont_Make(struct FontDesc* desc, const cc_string* fontName, int size, int flags) {
	size *= DisplayInfo.ScaleY;
	/* Round upwards to nearest 8 */
	size = (size + 7) & ~0x07;

	desc->size   = size;
	desc->flags  = flags;
	desc->height = Drawer2D_AdjHeight(size);

	desc->handle = (void*)(size / 8);
	return 0;
}

void SysFont_MakeDefault(struct FontDesc* desc, int size, int flags) {
	SysFont_Make(desc, NULL, size, flags);
}

void SysFont_Free(struct FontDesc* desc) {
	desc->handle = NULL;
}


static int CellWidth(cc_uint8* rows) {
	int y, width, widest = 0;

	for (y = 0; y < CELL_SIZE; y++) 
	{
		widest = max(widest, rows[y]);
	}
	width = Math_ilog2(widest) + 1;

	return width + 1; /* add 1 for padding */
}

int SysFont_TextWidth(struct DrawTextArgs* args) {
	cc_string left = args->text, part;
	int scale = (int)args->font->handle;
	char colorCode = 'f';
	int i, width = 0;

	while (Drawer2D_UNSAFE_NextPart(&left, &part, &colorCode))
	{
		for (i = 0; i < part.length; i++) 
		{
			cc_uint8 c = part.buffer[i];
			if (c == ' ') {
				width += SPACE_WIDTH * scale;
			} else {
				width += CellWidth(SysFont_GetRows(c)) * scale;
			}
		}
	}

	width = max(1, width);
	if (args->useShadow) width += 1 * scale;
	return width;
}

static void DrawCell(struct Bitmap* bmp, int x, int y, 
					int scale, cc_uint8 * rows, BitmapCol color) {
	int dst_width  = CELL_SIZE * scale;
	int dst_height = CELL_SIZE * scale;
	int xx, srcX, dstX;
	int yy, srcY, dstY;
	BitmapCol* dst_row;
	cc_uint8 src_row;

	for (yy = 0; yy < dst_height; yy++)
	{
		srcY = yy / scale;
		dstY = y + yy;
		if (dstY < 0 || dstY >= bmp->height) continue;

		dst_row = Bitmap_GetRow(bmp, dstY);
		src_row = rows[srcY];

		for (xx = 0; xx < dst_width; xx++)
		{
			srcX = xx / scale;
			dstX = x + xx;
			if (dstX < 0 || dstX >= bmp->width) continue;

			if (src_row & (1 << srcX)) {
				dst_row[dstX] = color;
			}
		}
	}
}

void SysFont_DrawText(struct DrawTextArgs* args, struct Bitmap* bmp, int x, int y, cc_bool shadow) {
	cc_string left = args->text, part;
	int scale = (int)args->font->handle;
	int size  = args->font->size;
	char colorCode = 'f';
	cc_uint8* rows;
	BitmapCol color;
	int i;

	if (shadow) { 
		x += 1 * scale; 
		y += 1 * scale;
	}

	/* adjust coords to make drawn text match GDI fonts */
	y += (args->font->height - size) / 2;

	while (Drawer2D_UNSAFE_NextPart(&left, &part, &colorCode))
	{
		color = Drawer2D_GetColor(colorCode);
		if (shadow) color = GetShadowColor(color);
	
		for (i = 0; i < part.length; i++) 
		{
			cc_uint8 c = part.buffer[i];
			if (c == ' ') { x += SPACE_WIDTH * scale; continue; }

			rows = SysFont_GetRows(c);

			DrawCell(bmp, x, y, scale, rows, color);
			x += CellWidth(rows) * scale;
		}
	}
}
#endif
