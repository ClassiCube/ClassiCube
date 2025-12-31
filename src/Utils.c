#include "Utils.h"
#include "String_.h"
#include "Bitmap.h"
#include "Platform.h"
#include "Stream.h"
#include "Errors.h"
#include "Logger.h"


/*########################################################################################################################*
*----------------------------------------------------------Misc-----------------------------------------------------------*
*#########################################################################################################################*/
int Utils_ParseEnum(const cc_string* text, int defValue, const char* const* names, int namesCount) {
	int i;
	for (i = 0; i < namesCount; i++) {
		if (String_CaselessEqualsConst(text, names[i])) return i;
	}
	return defValue;
}

cc_bool Utils_IsUrlPrefix(const cc_string* value) {
	return String_IndexOfConst(value, "http://")  == 0
		|| String_IndexOfConst(value, "https://") == 0;
}

cc_bool Utils_EnsureDirectory(const char* dirName) {
	cc_filepath raw_dir;
	cc_string dir;
	cc_result res;
	
	dir = String_FromReadonly(dirName);
	Platform_EncodePath(&raw_dir, &dir);
	res = Directory_Create2(&raw_dir);

	if (!res || res == ReturnCode_DirectoryExists) return true;
	Logger_IOWarn2(res, "creating directory", &raw_dir);
	return false;
}

void Utils_UNSAFE_GetFilename(STRING_REF cc_string* path) {
	char c;
	int i;

	for (i = path->length - 1; i >= 0; i--) {
		c = path->buffer[i];
		if (c == '/' || c == '\\') { 
			*path = String_UNSAFE_SubstringAt(path, i + 1); return; 
		}
	}
}

void Utils_UNSAFE_TrimFirstDirectory(STRING_REF cc_string* path) {
	char c;
	int i;

	for (i = 0; i < path->length; i++) {
		c = path->buffer[i];
		if (c == '/' || c == '\\') {
			*path = String_UNSAFE_SubstringAt(path, i + 1); return;
		}
	}
}


int Utils_AccumulateWheelDelta(float* accumulator, float delta) {
	int steps;
	/* Some mice may use deltas of say (0.2, 0.3, 0.4, 0.1) */
	/* We must use rounding at final step, not at every intermediate step. */
	*accumulator += delta;
	steps = (int)(*accumulator);
	*accumulator -= steps;
	return steps;
}

/* Checks if an area is completely black, so Alex skins edited with Microsoft Paint are still treated as Alex */
static cc_bool IsAllBlack(const struct Bitmap* bmp, int x1, int y1, int width, int height) {
	int x, y;
	for (y = y1; y < y1 + height; y++) {
		BitmapCol* row = Bitmap_GetRow(bmp, y);

		for (x = x1; x < x1 + width; x++) {
			if (row[x] != BITMAPCOLOR_BLACK) return false;
		}
	}
	return true;
}

cc_uint8 Utils_CalcSkinType(const struct Bitmap* bmp) {
	BitmapCol col;
	int scale;
	if (bmp->width == bmp->height * 2) return SKIN_64x32;
	if (bmp->width != bmp->height)     return SKIN_INVALID;

	scale = bmp->width / 64;
	/* Minecraft alex skins have this particular pixel with alpha of 0 */	
	col = Bitmap_GetPixel(bmp, 54 * scale, 20 * scale);
	if (BitmapCol_A(col) < 128) return SKIN_64x64_SLIM;

	return IsAllBlack(bmp, 54 * scale, 20 * scale, 2 * scale, 12 * scale)
		&& IsAllBlack(bmp, 50 * scale, 16 * scale, 2 * scale,  4 * scale) ? SKIN_64x64_SLIM : SKIN_64x64;
}

cc_uint32 Utils_CRC32(const cc_uint8* data, cc_uint32 length) {
	cc_uint32 crc = 0xffffffffUL;
	int i;

	for (i = 0; i < length; i++) {
		crc = Utils_Crc32Table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
	}
	return crc ^ 0xffffffffUL;
}

const cc_uint32 Utils_Crc32Table[256] = {
	0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
	0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7, 0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
	0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
	0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
	0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433, 0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
	0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
	0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
	0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F, 0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
	0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
	0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
	0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B, 0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
	0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
	0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
	0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777, 0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
	0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
	0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D,
};

void Utils_Resize(void** buffer, int* capacity, cc_uint32 elemSize, int defCapacity, int expandElems) {
	/* We use a statically allocated buffer initially, so can't realloc first time */
	int curCapacity = *capacity, newCapacity = curCapacity + expandElems;
	*capacity = newCapacity;

	if (curCapacity <= defCapacity) {
		void* resized = Mem_Alloc(newCapacity, elemSize, "initing array");
		Mem_Copy(resized, *buffer, (cc_uint32)curCapacity * elemSize);
		*buffer = resized;
	} else {
		*buffer = Mem_Realloc(*buffer, newCapacity, elemSize, "resizing array");
	}
}

void Utils_SwapEndian16(cc_int16* values, int numValues) {
	cc_uint8* data = (cc_uint8*)values;
	int i;

	for (i = 0; i < numValues * 2; i += 2)
	{
		cc_uint8 tmp = data[i + 0];
		data[i + 0]  = data[i + 1];
		data[i + 1]  = tmp;
	}
}


static const char base64_table[64] = {
	'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
	'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
	'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
	'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/'
};
int Convert_ToBase64(const void* data, int len, char* dst) {
	const cc_uint8* src = (const cc_uint8*)data;
	char* beg = dst;

	/* 3 bytes to 4 chars */
	for (; len >= 3; len -= 3, src += 3) {
		*dst++ = base64_table[                         (src[0] >> 2)];
		*dst++ = base64_table[((src[0] & 0x03) << 4) | (src[1] >> 4)];
		*dst++ = base64_table[((src[1] & 0x0F) << 2) | (src[2] >> 6)];
		*dst++ = base64_table[((src[2] & 0x3F))];
	}

	switch (len) {
	case 1:
		*dst++ = base64_table[                         (src[0] >> 2)];
		*dst++ = base64_table[((src[0] & 0x03) << 4)                ];
		*dst++ = '=';
		*dst++ = '=';
		break;
	case 2:
		*dst++ = base64_table[                         (src[0] >> 2)];
		*dst++ = base64_table[((src[0] & 0x03) << 4) | (src[1] >> 4)];
		*dst++ = base64_table[((src[1] & 0x0F) << 2)                ];
		*dst++ = '=';
		break;
	}
	return (int)(dst - beg);
}

/* Maps a base 64 character back into a 6 bit integer */
CC_NOINLINE static int DecodeBase64(char c) {
	if (c >= 'A' && c <= 'Z') return (c - 'A');
	if (c >= 'a' && c <= 'z') return (c - 'a') + 26;
	if (c >= '0' && c <= '9') return (c - '0') + 52;
	
	if (c == '+') return 62;
	if (c == '/') return 63;
	return -1;
}

int Convert_FromBase64(const char* src, int len, cc_uint8* dst) {
	cc_uint8* beg = dst;
	int a, b, c, d;
	/* base 64 must be padded with = to 4 characters */
	if (len & 0x3) return 0;

	/* 4 chars to 3 bytes */
	/* stops on any invalid chars (also handles = padding) */
	for (; len >= 4; len -= 4, src += 4) {
		a = DecodeBase64(src[0]);
		b = DecodeBase64(src[1]);
		if (a == -1 || b == -1) break;
		*dst++ = (a << 2) | (b >> 4);

		c = DecodeBase64(src[2]);
		if (c == -1) break;
		*dst++ = (b << 4) | (c >> 2);

		d = DecodeBase64(src[3]);
		if (d == -1) break;
		*dst++ = (c << 6) | (d     );
	}
	return (int)(dst - beg);
}


/*########################################################################################################################*
*--------------------------------------------------------EntryList--------------------------------------------------------*
*#########################################################################################################################*/
cc_result EntryList_Load(struct StringsBuffer* list, const char* file, char separator, EntryList_Filter filter) {
	cc_string entry; char entryBuffer[1024];
	cc_string path;
	cc_string key, value;
	int lineLen, maxLen;

	cc_uint8 buffer[2048];
	struct Stream stream, buffered;
	cc_filepath raw_path;
	cc_result res;

	path   = String_FromReadonly(file);
	maxLen = list->_lenMask ? list->_lenMask : STRINGSBUFFER_DEF_LEN_MASK;
	
	Platform_EncodePath(&raw_path, &path);
	res = Stream_OpenPath(&stream, &raw_path);
	if (res == ReturnCode_FileNotFound) return res;
	if (res) { Logger_IOWarn2(res, "opening", &raw_path); return res; }

	/* ReadLine reads single byte at a time */
	Stream_ReadonlyBuffered(&buffered, &stream, buffer, sizeof(buffer));
	for (;;) {
		/* Must be re-initialised each time as String_UNSAFE_TrimStart adjusts entry.buffer */
		String_InitArray(entry, entryBuffer);

		res = Stream_ReadLine(&buffered, &entry);
		if (res == ERR_END_OF_STREAM) break;
		if (res) { Logger_IOWarn2(res, "reading from", &raw_path); break; }
		
		/* whitespace lines are ignored (e.g. if user manually edits file) */
		String_UNSAFE_TrimStart(&entry);
		String_UNSAFE_TrimEnd(&entry);

		if (!entry.length) continue;
		if (filter && !filter(&entry)) continue;

		/* Sometimes file becomes corrupted and replaced with NULL */
		/* If don't prevent this here, client aborts in StringsBuffer_Add */
		if (entry.length > maxLen) {
			lineLen      = entry.length;
			entry.length = 0;
			String_Format2(&entry, "Skipping very long (%i characters) line in %c, file may be corrupted", &lineLen, file);
			Logger_WarnFunc(&entry); continue;
		}

		if (separator) {
			String_UNSAFE_Separate(&entry, separator, &key, &value);
			EntryList_Set(list, &key, &value, separator);
		} else {
			StringsBuffer_Add(list, &entry);
		}
	}

	/* No point logging error for closing readonly file */
	(void)stream.Close(&stream);
	return res;
}

cc_result EntryList_UNSAFE_Load(struct StringsBuffer* list, const char* file) { 
	return EntryList_Load(list, file, '\0', NULL); 
}

void EntryList_Save(struct StringsBuffer* list, const char* file) {
	cc_string path, entry; char pathBuffer[FILENAME_SIZE];
	struct Stream stream;
	cc_filepath raw_path;
	cc_result res;
	int i;

	String_InitArray(path, pathBuffer);
	String_AppendConst(&path, file);
	
	Platform_EncodePath(&raw_path, &path);
	res = Stream_CreatePath(&stream, &raw_path);
	if (res) { Logger_IOWarn2(res, "creating", &raw_path); return; }

	for (i = 0; i < list->count; i++) {
		StringsBuffer_UNSAFE_GetRaw(list, i, &entry);
		res = Stream_WriteLine(&stream, &entry);
		if (res) { Logger_IOWarn2(res, "writing to", &raw_path); break; }
	}

	res = stream.Close(&stream);
	if (res) { Logger_IOWarn2(res, "closing", &raw_path); }
}

cc_bool EntryList_Remove(struct StringsBuffer* list, const cc_string* key, char separator) {
	cc_bool found = false;
	/* Have to use a for loop, because may be multiple entries with same key */
	for (;;) {
		int i = EntryList_Find(list, key, separator);
		if (i == -1) break;
		
		StringsBuffer_Remove(list, i);
		found = true;
	}
	return found;
}

void EntryList_Set(struct StringsBuffer* list, const cc_string* key, const cc_string* value, char separator) {
	cc_string entry; char entryBuffer[3072];
	String_InitArray(entry, entryBuffer);

	if (value->length) {
		String_Format3(&entry, "%s%r%s", key, &separator, value);
	} else {
		String_Copy(&entry, key);
	}

	EntryList_Remove(list, key, separator);
	StringsBuffer_Add(list, &entry);
}

cc_string EntryList_UNSAFE_Get(struct StringsBuffer* list, const cc_string* key, char separator) {
	cc_string curEntry, curKey, curValue;
	int i;

	for (i = 0; i < list->count; i++) {
		StringsBuffer_UNSAFE_GetRaw(list, i, &curEntry);
		String_UNSAFE_Separate(&curEntry, separator, &curKey, &curValue);

		if (String_CaselessEquals(key, &curKey)) return curValue;
	}
	return String_Empty;
}

int EntryList_Find(struct StringsBuffer* list, const cc_string* key, char separator) {
	cc_string curEntry, curKey, curValue;
	int i;

	for (i = 0; i < list->count; i++) {
		StringsBuffer_UNSAFE_GetRaw(list, i, &curEntry);
		String_UNSAFE_Separate(&curEntry, separator, &curKey, &curValue);

		if (String_CaselessEquals(key, &curKey)) return i;
	}
	return -1;
}

