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
	String_FromConst("Liberation Sans"), /* ice looking fallbacks for linux */
	String_FromConst("Nimbus Sans"),
	String_FromConst("Bitstream Charter"),
	String_FromConst("Cantarell"),
	String_FromConst("DejaVu Sans Book"), 
	String_FromConst("Century Schoolbook L Roman"), /* commonly available on linux */
	String_FromConst("Liberation Serif"), /* for SerenityOS */
	String_FromConst("Slate For OnePlus"), /* Android 10, some devices */
	String_FromConst("Roboto"), /* Android (broken on some Android 10 devices) */
	String_FromConst("Geneva"), /* for ancient macOS versions */
	String_FromConst("Droid Sans") /* for old Android versions */
};

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
	/* Need to keep track of whether font cache has been checked at least once */
	/* (Otherwise if unable to find any cached fonts and then unable to load any fonts, */
	/*  font_list.count will always be 0 and the 'Initialising font cache' dialog will 
	    confusingly get shown over and over until all font_candidates entries are checked) */
	static cc_bool checkedCache;
	if (checkedCache) return;
	checkedCache = true;

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
	cc_string path;
	if (!font_list.count) SysFonts_Load();
	path = String_Empty;

	if (flags & FONT_FLAGS_BOLD) path = Font_LookupOf(fontName, 'B');
	return path.length ? path : Font_LookupOf(fontName, 'R');
}

static cc_string Font_Lookup(const cc_string* fontName, int flags) {
	cc_string path = Font_DoLookup(fontName, flags);
	if (path.length) return path;

	SysFonts_Update();
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

void SysFont_MakeDefault(struct FontDesc* desc, int size, int flags) {
	cc_string* font;
	cc_result res;
	int i;
	font_candidates[0] = font_default;

	for (i = 0; i < Array_Elems(font_candidates); i++) {
		font = &font_candidates[i];
		if (!font->length) continue;
		res  = SysFont_Make(desc, &font_candidates[i], size, flags);

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
#elif defined CC_BUILD_PSP || defined CC_BUILD_GCWII
void SysFonts_Register(const cc_string* path) { }

const cc_string* SysFonts_UNSAFE_GetDefault(void) { return &String_Empty; }

void SysFonts_GetNames(struct StringsBuffer* buffer) { }

cc_result SysFont_Make(struct FontDesc* desc, const cc_string* fontName, int size, int flags) {
	desc->size   = size;
	desc->flags  = flags;
	desc->height = Drawer2D_AdjHeight(size);

	desc->handle = (void*)1;
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
	u8 a, I, invI;

	int rowY = sheetPos / tglp->nRows;
	int rowX = sheetPos % tglp->nRows;
	
	charWidthInfo_s* wInfo = fontGetCharWidthInfo(font, glyphIndex);	
	//int L = wInfo->left, W = wInfo->glyphWidth;
	//Platform_Log3("Draw %r (L=%i, W=%i", &CP, &L, &W);
	
	// TODO not very efficient.. but it works I guess
	// Can this be rewritten to use normal Drawer2D's bitmap font rendering somehow?
	for (int Y = 0; Y < tglp->cellHeight; Y++)
	{
		for (int X = 0; X < wInfo->glyphWidth; X++)
		{
			int dstX = x + X + wInfo->left, dstY = y + Y;
			if (dstX < 0 || dstY < 0 || dstX >= bmp->width || dstY >= bmp->height) continue;
			
			int srcX = X + rowX * (tglp->cellWidth  + 1);
			int srcY = Y + rowY * (tglp->cellHeight + 1);
			
			int tile_offset   = (srcY & ~0x07) * tglp->sheetWidth + (srcX & ~0x07) * 8;
			int tile_location = CalcMortonOffset(srcX & 0x07, srcY & 0x07);
			
			// each byte stores two pixels in it
			a = sheet[(tile_offset + tile_location) >> 1];
			a = (tile_location & 1) ? (a >> 4) : (a & 0x0F);
			a = a * 0x11; // 0-15 > 0-255
			
			I = a; invI = UInt8_MaxValue - a;

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
#endif
