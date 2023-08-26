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
#elif defined CC_BUILD_PSP
void SysFonts_Register(const cc_string* path) { }

const cc_string* SysFonts_UNSAFE_GetDefault(void) { return &String_Empty; }

void SysFonts_GetNames(struct StringsBuffer* buffer) { }

cc_result SysFont_Make(struct FontDesc* desc, const cc_string* fontName, int size, int flags) {
	desc->size   = size;
	desc->flags  = flags;
	desc->height = Drawer2D_AdjHeight(size);

	desc->handle = (void*)1;
	
	// TODO: Actually implement native font APIs
	Font_MakeBitmapped(desc, size, flags);
	return 0;
}

void SysFont_MakeDefault(struct FontDesc* desc, int size, int flags) {
	SysFont_Make(desc, NULL, size, flags);
}

void SysFont_Free(struct FontDesc* desc) {
}

int SysFont_TextWidth(struct DrawTextArgs* args) {
	return 10;
}

void SysFont_DrawText(struct DrawTextArgs* args, struct Bitmap* bmp, int x, int y, cc_bool shadow) {
}
#elif defined CC_BUILD_GCWII
#include <ogc/system.h>
void SysFonts_Register(const cc_string* path) { }

const cc_string* SysFonts_UNSAFE_GetDefault(void) { return &String_Empty; }

void SysFonts_GetNames(struct StringsBuffer* buffer) { }

static union {
	sys_fontheader hdr;
	u8 data[SYS_FONTSIZE_ANSI];
} __attribute__((aligned(32))) ipl_font;
// must be 32 byte aligned for correct font reading

static cc_bool font_checked, font_okay;

static void LoadIPLFont(void) {
	font_checked = true;
	// SJIS font layout not supported
	if (SYS_GetFontEncoding() == 1) return;
	
	int res   = SYS_InitFont(&ipl_font.hdr);
	font_okay = res == 1;
}

cc_result SysFont_Make(struct FontDesc* desc, const cc_string* fontName, int size, int flags) {
	if (!font_checked) LoadIPLFont();

	desc->size   = size;
	desc->flags  = flags;
	desc->height = ipl_font.hdr.cell_height;
	desc->handle = (void*)1;	
	
	if (!font_okay) Font_MakeBitmapped(desc, size, flags);
	return 0;
}

void SysFont_MakeDefault(struct FontDesc* desc, int size, int flags) {
	SysFont_Make(desc, NULL, size, flags);
}

void SysFont_Free(struct FontDesc* desc) {
}

int SysFont_TextWidth(struct DrawTextArgs* args) {
	// TODO: Optimise 
	// u8* width_table = &ipl_font.data[ipl_font.header.width_table];
	cc_string left = args->text, part;
	char colorCode = 'f';	
	int width = 0;
	
	void* image;
	s32 cellX, cellY, glyphWidth;

	while (Drawer2D_UNSAFE_NextPart(&left, &part, &colorCode))
	{
		for (int i = 0; i < part.length; i++) 
		{
			SYS_GetFontTexture((u8)part.buffer[i], &image, 
						&cellX, &cellY, &glyphWidth);
			width += glyphWidth;
		}
	}
	return max(1, width);
}
// https://devkitpro.org/viewtopic.php?f=7&t=191
//  Re: SYS_GetFontTexture & SYS_GetFontTexel
static int DrawGlyph(struct Bitmap* bmp, int x, int y, u8 c, BitmapCol color) {
	void* image;
	s32 cellX, cellY, glyphWidth;
	SYS_GetFontTexture(c, &image, &cellX, &cellY, &glyphWidth);
	// after having been decompressed back in SYS_InitFont, 
	//  font pixels are represented using I4 texture format
		
	int cellWidth  = ipl_font.hdr.cell_width;
	int cellHeight = ipl_font.hdr.cell_height;
	int sheetWidth = ipl_font.hdr.sheet_width;
	u8 I, invI;
	// TODO not very efficient.. but it works I guess
	// Can this be rewritten to use normal Drawer2D's bitmap font rendering somehow?
	for (int glyphY = 0; glyphY < cellHeight; glyphY++) // glyphX, cellX
	{
		int dstY = glyphY + y;
		int srcY = glyphY + cellY;
		if (dstY < 0 || dstY >= bmp->height) continue;		
			
		for (int glyphX = 0; glyphX < glyphWidth; glyphX++)
		{
			int dstX = glyphX + x;
			int srcX = glyphX + cellX;
			if (dstX < 0 || dstX >= bmp->width) continue;
			
			// The I4 texture is divided into 8x8 blocks
			//   https://wiki.tockdom.com/wiki/Image_Formats#I4
			int tileX = srcX & ~0x07;
                	int tileY = srcY & ~0x07;               	
                	int tile_offset   = (tileY * sheetWidth) + (tileX << 3);
                	int tile_location = ((srcY & 7) << 3) | (srcX & 7);
                	
                	// each byte stores two pixels in it
                	I = ((u8*)image)[(tile_offset | tile_location) >> 1];
			I = (glyphX & 1) ? (I & 0x0F) : (I >> 4);
			I = I * 0x11; // 0-15 > 0-255
			
			if (!I) continue;
			invI = UInt8_MaxValue - I;
			
			BitmapCol src = Bitmap_GetPixel(bmp, dstX, dstY);
			Bitmap_GetPixel(bmp, dstX, dstY) = BitmapCol_Make(
				((BitmapCol_R(color) * I) >> 8) + ((BitmapCol_R(src) * invI) >> 8),
				((BitmapCol_G(color) * I) >> 8) + ((BitmapCol_G(src) * invI) >> 8),
				((BitmapCol_B(color) * I) >> 8) + ((BitmapCol_B(src) * invI) >> 8),
				                             I  + ((BitmapCol_A(src) * invI) >> 8));
		}
	}
	return glyphWidth;
}

void SysFont_DrawText(struct DrawTextArgs* args, struct Bitmap* bmp, int x, int y, cc_bool shadow) {
	cc_string left = args->text, part;
	char colorCode = 'f';
	BitmapCol color;

	while (Drawer2D_UNSAFE_NextPart(&left, &part, &colorCode))
	{
		color = Drawer2D_GetColor(colorCode);
		if (shadow) color = GetShadowColor(color);
	
		for (int i = 0; i < part.length; i++) 
		{
			x += DrawGlyph(bmp, x, y, (u8)part.buffer[i], color);
		}
	}
}
#elif defined CC_BUILD_3DS
#include <3ds.h>

void SysFonts_Register(const cc_string* path) { }

const cc_string* SysFonts_UNSAFE_GetDefault(void) { return &String_Empty; }

void SysFonts_GetNames(struct StringsBuffer* buffer) { }

cc_result SysFont_Make(struct FontDesc* desc, const cc_string* fontName, int size, int flags) {
	desc->size   = size;
	desc->flags  = flags;
	desc->height = Drawer2D_AdjHeight(size);
	desc->handle = fontGetSystemFont();
	
	CFNT_s* font = (CFNT_s*)desc->handle;
	int fmt = font->finf.tglp->sheetFmt;
	Platform_Log1("Font GPU format: %i", &fmt);
	return 0;
}

void SysFont_MakeDefault(struct FontDesc* desc, int size, int flags) {
	SysFont_Make(desc, NULL, size, flags);
}

void SysFont_Free(struct FontDesc* desc) {
}

int SysFont_TextWidth(struct DrawTextArgs* args) {
	int width = 0;
	cc_string left = args->text, part;
	char colorCode = 'f';
	CFNT_s* font   = (CFNT_s*)args->font->handle;

	while (Drawer2D_UNSAFE_NextPart(&left, &part, &colorCode))
	{
		for (int i = 0; i < part.length; i++) 
		{
			cc_unichar cp  = Convert_CP437ToUnicode(part.buffer[i]);
			int glyphIndex = fontGlyphIndexFromCodePoint(font, cp);
			if (glyphIndex < 0) continue;
			
			charWidthInfo_s* wInfo = fontGetCharWidthInfo(font, glyphIndex);
			width += wInfo->charWidth;
		}
	}
	return max(1, width);
}


// see Graphics_3DS.c for more details
static inline cc_uint32 CalcMortonOffset(cc_uint32 x, cc_uint32 y) {
	// TODO: Simplify to array lookup?
    	x = (x | (x << 2)) & 0x33;
    	x = (x | (x << 1)) & 0x55;

    	y = (y | (y << 2)) & 0x33;
    	y = (y | (y << 1)) & 0x55;

    return x | (y << 1);
}

static void DrawGlyph(CFNT_s* font, struct Bitmap* bmp, int x, int y, int glyphIndex, int CP, BitmapCol color) {
	TGLP_s* tglp = font->finf.tglp;
	int fmt = font->finf.tglp->sheetFmt;
	if (fmt != GPU_A4) return;
	
	int glyphsPerSheet = tglp->nRows * tglp->nLines;
	int sheetIdx = glyphIndex / glyphsPerSheet;
	int sheetPos = glyphIndex % glyphsPerSheet;
	u8* sheet    = tglp->sheetData + (sheetIdx * tglp->sheetSize);
	u8 I, invI;

	int rowX = sheetPos % tglp->nRows;
	int rowY = sheetPos / tglp->nRows;
	
	int cellX = rowX * (tglp->cellWidth  + 1) + 1;
	int cellY = rowY * (tglp->cellHeight + 1) + 1;
	charWidthInfo_s* wInfo = fontGetCharWidthInfo(font, glyphIndex);	
	//int L = wInfo->left, W = wInfo->glyphWidth;
	//Platform_Log3("Draw %r (L=%i, W=%i", &CP, &L, &W);
	
	// TODO not very efficient.. but it works I guess
	// Can this be rewritten to use normal Drawer2D's bitmap font rendering somehow?
	for (int glyphY = 0; glyphY < tglp->cellHeight; glyphY++)
	{
		int dstY = glyphY + y;
		int srcY = glyphY + cellY;
		if (dstY < 0 || dstY >= bmp->height) continue;
		
		for (int glyphX = 0; glyphX < wInfo->glyphWidth; glyphX++)
		{
			int dstX = glyphX + x + wInfo->left;
			int srcX = glyphX + cellX;
			if (dstX < 0 || dstX >= bmp->width) continue;
			
			int tile_offset   = (srcY & ~0x07) * tglp->sheetWidth + (srcX & ~0x07) * 8;
			int tile_location = CalcMortonOffset(srcX & 0x07, srcY & 0x07);
			
			// each byte stores two pixels in it
			I = sheet[(tile_offset + tile_location) >> 1];
			I = (tile_location & 1) ? (I >> 4) : (I & 0x0F);
			I = I * 0x11; // 0-15 > 0-255
			
			if (!I) continue;
			invI = UInt8_MaxValue - I;

			BitmapCol src = Bitmap_GetPixel(bmp, dstX, dstY);
			Bitmap_GetPixel(bmp, dstX, dstY) = BitmapCol_Make(
				((BitmapCol_R(color) * I) >> 8) + ((BitmapCol_R(src) * invI) >> 8),
				((BitmapCol_G(color) * I) >> 8) + ((BitmapCol_G(src) * invI) >> 8),
				((BitmapCol_B(color) * I) >> 8) + ((BitmapCol_B(src) * invI) >> 8),
				                             I  + ((BitmapCol_A(src) * invI) >> 8)
			);
				
			/*Bitmap_GetPixel(bmp, dstX, dstY) = BitmapColor_RGB(
				((BitmapCol_R(color) * a) >> 8),
				((BitmapCol_G(color) * a) >> 8),
				((BitmapCol_B(color) * a) >> 8));*/
		}
	}
}

void SysFont_DrawText(struct DrawTextArgs* args, struct Bitmap* bmp, int x, int y, cc_bool shadow) {
	cc_string left = args->text, part;
	char colorCode = 'f';
	CFNT_s* font   = (CFNT_s*)args->font->handle;
	BitmapCol color;

	while (Drawer2D_UNSAFE_NextPart(&left, &part, &colorCode))
	{
		color = Drawer2D_GetColor(colorCode);
		if (shadow) color = GetShadowColor(color);
	
		for (int i = 0; i < part.length; i++) 
		{
			cc_unichar cp  = Convert_CP437ToUnicode(part.buffer[i]);
			int glyphIndex = fontGlyphIndexFromCodePoint(font, cp);
			if (glyphIndex < 0) continue;
			
			DrawGlyph(font, bmp, x, y, glyphIndex, cp, color);
			charWidthInfo_s* wInfo = fontGetCharWidthInfo(font, glyphIndex);
			x += wInfo->charWidth;
		}
	}
}
#elif defined CC_BUILD_DREAMCAST
#include <dc/biosfont.h>

void SysFonts_Register(const cc_string* path) { }

const cc_string* SysFonts_UNSAFE_GetDefault(void) { return &String_Empty; }

void SysFonts_GetNames(struct StringsBuffer* buffer) { }

cc_result SysFont_Make(struct FontDesc* desc, const cc_string* fontName, int size, int flags) {
	desc->size   = size;
	desc->flags  = flags;
	desc->height = BFONT_HEIGHT;

	desc->handle = (void*)1;
	return 0;
}

void SysFont_MakeDefault(struct FontDesc* desc, int size, int flags) {
	SysFont_Make(desc, NULL, size, flags);
}

void SysFont_Free(struct FontDesc* desc) {
}

int SysFont_TextWidth(struct DrawTextArgs* args) {
	int width = 0;
	cc_string left = args->text, part;
	char colorCode = 'f';

	while (Drawer2D_UNSAFE_NextPart(&left, &part, &colorCode))
	{
		width += part.length * BFONT_THIN_WIDTH;
	}
	return max(1, width);
}

static void DrawSpan(struct Bitmap* bmp, int x, int y, int row, BitmapCol color) {	
	if (y < 0 || y >= bmp->height) return;
	
	for (int glyphX = 0; glyphX < BFONT_THIN_WIDTH; glyphX++)
	{
		int dstX = x + glyphX;
		if (dstX < 0 || dstX >= bmp->width) continue;
			
		// row encodes 12 "1 bit values", starting from hi to lo
		if (row & (0x800 >> glyphX)) {
			Bitmap_GetPixel(bmp, dstX, y) = color;
		}
	}
}


static void DrawGlyph(struct Bitmap* bmp, int x, int y, uint8* cell, BitmapCol color) {
	// Each font glyph row contains 12 "1 bit" values horizontally
	// 	as 3 bytes = 24 bits, it therefore encodes 2 rows	
	for (int glyphY = 0; glyphY < BFONT_HEIGHT; glyphY += 2, y += 2, cell += 3)
	{
		int row1 = (cell[0] << 4) | ((cell[1] >> 4) & 0x0f);
		DrawSpan(bmp, x, y + 0, row1, color);
		
		int row2 = ((cell[1] & 0x0f) << 8) | cell[2];
		DrawSpan(bmp, x, y + 1, row2, color);
	}
}

void SysFont_DrawText(struct DrawTextArgs* args, struct Bitmap* bmp, int x, int y, cc_bool shadow) {
	cc_string left = args->text, part;
	char colorCode = 'f';
	BitmapCol color;

	while (Drawer2D_UNSAFE_NextPart(&left, &part, &colorCode))
	{
		color = Drawer2D_GetColor(colorCode);
		if (shadow) color = GetShadowColor(color);
	
		for (int i = 0; i < part.length; i++) 
		{
			uint8* cell = bfont_find_char((uint8)part.buffer[i]);
			
			DrawGlyph(bmp, x, y, cell, color);
			x += BFONT_THIN_WIDTH;
		}
	}
}
#elif defined CC_BUILD_PSVITA
#include <vitasdk.h>

struct SysFont {
	ScePvfFontId fontID;
};
#define TEXT_CEIL(x) (((x) + 63) >> 6)


static void* lib_handle;
static int inited;
#define ALIGNUP4(x) (((x) + 3) & ~0x03)

static void* Pvf_Alloc(void* userdata, unsigned int size) {
	return Mem_TryAlloc(ALIGNUP4(size), 1);
}

static void* Pvf_Realloc(void* userdata, void* old_ptr, unsigned int size) {
	return Mem_TryRealloc(old_ptr, ALIGNUP4(size), 1);
}

static void Pvf_Free(void* userdata, void* ptr) {
	Mem_Free(ptr);
}

static void InitPvfLib(void) {
	if (inited) return;
	inited = true;
	
	ScePvfInitRec params = { 
		NULL, SCE_PVF_MAX_OPEN, NULL, NULL,
		Pvf_Alloc, Pvf_Realloc, Pvf_Free
	};

	ScePvfError error = 0;
	lib_handle = scePvfNewLib(&params, &error);
	if (error) {
		Platform_Log1("PVF ERROR: %i", &error);
		return;
	}
	
	for (int i = 0; i < 128; i++) 
	{
		ScePvfFontStyleInfo fs;
		error = scePvfGetFontInfoByIndexNumber(lib_handle, &fs, i);
		if (error) { 
			Platform_Log1("PVF F ERROR: %i", &error); continue;
		}
		
		Platform_Log4("FONT %i = %c / %c / %c", &i, fs.fontName, fs.styleName, fs.fileName);
		
		int FC = fs. familyCode;
		int ST = fs. style;
		int LC = fs. languageCode;
		int RC = fs. regionCode;
		int CC = fs. countryCode;
		
		Platform_Log2("F_STYLE: %i, %i", &FC, &ST);
		Platform_Log3("F_CODES: %i, %i, %i", &LC, &RC, &CC);
		
	}
}

void SysFonts_Register(const cc_string* path) { }

const cc_string* SysFonts_UNSAFE_GetDefault(void) { return &String_Empty; }

void SysFonts_GetNames(struct StringsBuffer* buffer) {
	InitPvfLib();
	if (!lib_handle) return;
	
	ScePvfError error = 0;
	int count = scePvfGetNumFontList(lib_handle, &error);
	if (error) return;
	
	for (int i = 0; i < count; i++) 
	{
		ScePvfFontStyleInfo fs;
		error = scePvfGetFontInfoByIndexNumber(lib_handle, &fs, i);		
		if (error) { Platform_Log1("PVF ERROR: %i", &error); continue; }
		
		if (fs.languageCode == SCE_PVF_LANGUAGE_LATIN) {
			cc_string name = String_FromRawArray(fs.fileName);
			StringsBuffer_Add(buffer, &name);
		}
	}
}

static ScePvfFontIndex FindFontByName(const cc_string* fontName) {
	ScePvfError error = 0;
	int count = scePvfGetNumFontList(lib_handle, &error);
	if (error) return -1;
	
	for (int i = 0; i < count; i++) 
	{
		ScePvfFontStyleInfo fs;
		error = scePvfGetFontInfoByIndexNumber(lib_handle, &fs, i);		
		if (error) { Platform_Log1("PVF FBN ERROR: %i", &error); continue; }
		
		cc_string name = String_FromRawArray(fs.fileName);
		if (String_CaselessEquals(fontName, &name)) return i;
	}
	return -1;
}

static ScePvfFontIndex FindDefaultFont(void) {
	ScePvfFontStyleInfo style = { 0 };
	style.languageCode = SCE_PVF_LANGUAGE_LATIN;

	ScePvfError error = 0;
	int index = scePvfFindOptimumFont(lib_handle, &style, &error);
	
	if (error) { Platform_Log1("PVF FDF ERROR: %i", &error); index = -1; }
	return index;
}


static cc_result MakeSysFont(struct FontDesc* desc, const cc_string* fontName, int size, int flags) {
	desc->size   = size;
	desc->flags  = flags;
	desc->height = Drawer2D_AdjHeight(size);

	InitPvfLib();
	if (!lib_handle) return ERR_NOT_SUPPORTED;
	
	struct SysFont* font = Mem_AllocCleared(sizeof(struct SysFont), 1, "font");
	desc->handle = font;
	
	ScePvfFontIndex idx = -1;
	if (fontName) {
		idx = FindFontByName(fontName);
	} else {
		idx = FindDefaultFont();
	}
	if (idx == -1) return ERR_INVALID_ARGUMENT;
	
	
	ScePvfError error = 0;
	font->fontID = scePvfOpen(lib_handle, idx, 1, &error);
	Platform_Log1("FONT ID: %i", &font->fontID);
	
	if (!error) {
		ScePvfFontInfo fontInfo = { 0 };
		int err2 = scePvfGetFontInfo(font->fontID, &fontInfo);
		if (!err2) {
			Platform_Log3("FONT METRICS: H %i, A %i, D %i", &fontInfo.maxIGlyphMetrics.height64,
				&fontInfo.maxIGlyphMetrics.ascender64,&fontInfo.maxIGlyphMetrics.descender64);
			Platform_Log3("FONT METRICS: X %i, Y %i, A %i", &fontInfo.maxIGlyphMetrics.horizontalBearingX64,
				&fontInfo.maxIGlyphMetrics.horizontalBearingY64,&fontInfo.maxIGlyphMetrics.horizontalAdvance64);
		}
	}
	
	if (!error) {
		float scale = size * 72.0f / 96.0f;
		scePvfSetCharSize(font->fontID, scale, scale);
	}
	return error;
}

cc_result SysFont_Make(struct FontDesc* desc, const cc_string* fontName, int size, int flags) {
	return MakeSysFont(desc, fontName, size, flags);
}

void SysFont_MakeDefault(struct FontDesc* desc, int size, int flags) {
	cc_result res = MakeSysFont(desc, NULL, size, flags);
	if (res) Logger_Abort2(res, "Failed to init default font");
}

void SysFont_Free(struct FontDesc* desc) {
	struct SysFont* font = (struct SysFont*)desc->handle;
	if (font->fontID) scePvfClose(font->fontID);
	Mem_Free(font);
}

int SysFont_TextWidth(struct DrawTextArgs* args) {
	struct SysFont* font = (struct SysFont*)args->font->handle;
	ScePvfCharInfo charInfo;
	
	cc_string left = args->text, part;
	char colorCode = 'f';	
	int width = 0;

	while (Drawer2D_UNSAFE_NextPart(&left, &part, &colorCode))
	{
		for (int i = 0; i < part.length; i++) 
		{
			// TODO optimise
			ScePvfError error = scePvfGetCharInfo(font->fontID, (cc_uint8)part.buffer[i], &charInfo);
			if (error) { Platform_Log1("PVF TW ERROR: %i", &error); continue; }
			
			width += charInfo.glyphMetrics.horizontalAdvance64;
		}
	}
	
	width = TEXT_CEIL(width);
	Platform_Log2("TEXT WIDTH: %i (%s)", &width, &args->text);
	if (args->useShadow) width += 2;
	return max(1, width);
}

// TODO optimise
static void RasteriseGlyph(ScePvfUserImageBufferRec* glyph, struct Bitmap* bmp, int x, int y) {
	cc_uint8* src = glyph->buffer;
	int glyphHeight = glyph->rect.height;
	int glyphWidth  = glyph->rect.width;
	
	for (int glyphY = 0; glyphY < glyphHeight; glyphY++)
	{
		int dstY = glyphY + y;
		if (dstY < 0 || dstY >= bmp->height) continue;		
			
		for (int glyphX = 0; glyphX < glyphWidth; glyphX++)
		{
			int dstX = glyphX + x;
			if (dstX < 0 || dstX >= bmp->width) continue;
			
			cc_uint8 I = src[glyphY * glyphWidth + glyphX];

			Bitmap_GetPixel(bmp, dstX, dstY) = BitmapCol_Make(I, I, I, 255);
		}
	}
}

// TODO optimise
// See https://freetype.org/freetype2/docs/glyphs/glyphs-3.html
static int DrawGlyph(struct SysFont* font, int size, struct Bitmap* bmp, int x, int y, cc_uint8 c) {
	ScePvfCharInfo charInfo;
	ScePvfIrect charRect;
	ScePvfError error;
			
	error = scePvfGetCharInfo(font->fontID, c, &charInfo);
	if (error) { Platform_Log1("PVF DG_GCI  ERROR: %i", &error); return 0; }

	error = scePvfGetCharImageRect(font->fontID, c, &charRect);
	if (error) { Platform_Log1("PVF DG_GCIR ERROR: %i", &error); return 0; }
	
	ScePvfUserImageBufferRec glyph = { 0 };
	cc_uint8* tmp = Mem_Alloc(charRect.width * charRect.height, 1, "temp font bitmap");
	//Mem_Set(tmp, 0x00, charRect.width * charRect.height);
	glyph.pixelFormat  = SCE_PVF_USERIMAGE_DIRECT8;
	glyph.rect.width   = charRect.width;
	glyph.rect.height  = charRect.height;
	glyph.bytesPerLine = charRect.width;
	glyph.buffer       = tmp;
	
	//glyph.xPos64 = -charInfo.glyphMetrics.horizontalBearingX64;
	//glyph.yPos64 = +charInfo.glyphMetrics.horizontalBearingY64;
	
	// TODO: use charInfo.glyphMetrics.horizontalBearingX64 and Y64
	Platform_Log1("ABOUT %r:", &c);
	int BX = charInfo.glyphMetrics.horizontalBearingX64, BX2 = TEXT_CEIL(BX);
	int BY = charInfo.glyphMetrics.horizontalBearingY64, BY2 = TEXT_CEIL(BY);
	//Platform_Log4("  Bitmap: %i,%i --> %i, %i", &charInfo.bitmapLeft, &charInfo.bitmapTop, &charInfo.bitmapWidth, &charInfo.bitmapHeight);
	
	int W = charInfo.glyphMetrics.width64,     W2 =TEXT_CEIL(W);
	int H = charInfo.glyphMetrics.height64,    H2 =TEXT_CEIL(H);
	int A = charInfo.glyphMetrics.ascender64,  A2 =TEXT_CEIL(A);
	int D = charInfo.glyphMetrics.descender64, D2 =TEXT_CEIL(D);
	
	Platform_Log4("  Size: %i,%i   (%i, %i)", &W, &H, &W2, &H2);
	Platform_Log4("  Vert: %i,%i   (%i, %i)", &A, &D, &A2, &D2);
	Platform_Log4("  Bear: %i,%i   (%i, %i)", &BX, &BY, &BX2, &BY2);
			
	int CW = charRect.width, CH = charRect.height;
	Platform_Log2("  CharSize: %i,%i", &CW, &CH);
	
	if (A2 < size) y += (size - A2);
	
	error = scePvfGetCharGlyphImage(font->fontID, c, &glyph);
	if (!error) RasteriseGlyph(&glyph, bmp, x, y);
	Mem_Free(tmp);
	if (error) { Platform_Log1("PVF DG_CCGI ERROR: %i", &error); return 0; }
	
	return TEXT_CEIL(charInfo.glyphMetrics.horizontalAdvance64);
}

// TODO better shadow support
void SysFont_DrawText(struct DrawTextArgs* args, struct Bitmap* bmp, int x, int y, cc_bool shadow) {
	struct SysFont* font = (struct SysFont*)args->font->handle;
	if (shadow) return;//{ x += 2; y += 2; }
	
	int W = SysFont_TextWidth(args);
	int S = args->font->size;
	int H = args->font->height;
	Platform_Log3("TOTAL: %i  (%i/%i)", &W, &S, &H);
	
	cc_string left = args->text, part;
	char colorCode = 'f';
	BitmapCol color;

	while (Drawer2D_UNSAFE_NextPart(&left, &part, &colorCode))
	{
		color = Drawer2D_GetColor(colorCode);
		if (shadow) color = GetShadowColor(color);
	
		for (int i = 0; i < part.length; i++) 
		{
			x += DrawGlyph(font, S, bmp, x, y, (cc_uint16)part.buffer[i]);		
		}
	}
}
#endif
