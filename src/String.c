#include "String.h"
#include "Funcs.h"
#include "Logger.h"
#include "Platform.h"
#include "Stream.h"
#include "Utils.h"

#ifdef __cplusplus
const cc_string String_Empty = { NULL, 0, 0 };
#else
const cc_string String_Empty;
#endif

int String_CalcLen(const char* raw, int capacity) {
	int length = 0;
	while (length < capacity && *raw) { raw++; length++; }
	return length;
}

int String_Length(const char* raw) {
	int length = 0;
	while (length < UInt16_MaxValue && *raw) { raw++; length++; }
	return length;
}

cc_string String_FromRaw(STRING_REF char* buffer, int capacity) {
	return String_Init(buffer, String_CalcLen(buffer, capacity), capacity);
}

cc_string String_FromReadonly(STRING_REF const char* buffer) {
	int len = String_Length(buffer);
	return String_Init((char*)buffer, len, len);
}


void String_Copy(cc_string* dst, const cc_string* src) {
	dst->length = 0;
	String_AppendString(dst, src);
}

void String_CopyToRaw(char* dst, int capacity, const cc_string* src) {
	int i, len = min(capacity, src->length);
	for (i = 0; i < len; i++) { dst[i] = src->buffer[i]; }
	/* add \0 to mark end of used portion of buffer */
	if (len < capacity) dst[len] = '\0';
}

cc_string String_UNSAFE_Substring(STRING_REF const cc_string* str, int offset, int length) {
	if (offset < 0 || offset > str->length) {
		Logger_Abort("Offset for substring out of range");
	}
	if (length < 0 || length > str->length) {
		Logger_Abort("Length for substring out of range");
	}
	if (offset + length > str->length) {
		Logger_Abort("Result substring is out of range");
	}
	return String_Init(str->buffer + offset, length, length);
}

cc_string String_UNSAFE_SubstringAt(STRING_REF const cc_string* str, int offset) {
	cc_string sub;
	if (offset < 0 || offset > str->length) Logger_Abort("Sub offset out of range");

	sub.buffer   = str->buffer + offset;
	sub.length   = str->length - offset;
	sub.capacity = str->length - offset; /* str->length to match String_UNSAFE_Substring */
	return sub;
}

int String_UNSAFE_Split(STRING_REF const cc_string* str, char c, cc_string* subs, int maxSubs) {
	int beg = 0, end, count, i;

	for (i = 0; i < maxSubs && beg <= str->length; i++) {
		end = String_IndexOfAt(str, beg, c);
		if (end == -1) end = str->length;

		subs[i] = String_UNSAFE_Substring(str, beg, end - beg);
		beg = end + 1;
	}

	count = i;
	/* If not enough split substrings, make remaining NULL */
	for (; i < maxSubs; i++) { subs[i] = String_Empty; }
	return count;
}

void String_UNSAFE_SplitBy(STRING_REF cc_string* str, char c, cc_string* part) {
	int idx = String_IndexOf(str, c);
	if (idx == -1) {
		*part = *str;
		*str  = String_Empty;
	} else {
		*part = String_UNSAFE_Substring(str, 0, idx); idx++;
		*str  = String_UNSAFE_SubstringAt(str, idx);
	}
}

int String_UNSAFE_Separate(STRING_REF const cc_string* str, char c, cc_string* key, cc_string* value) {
	int idx = String_IndexOf(str, c);
	if (idx == -1) {
		*key   = *str;
		*value = String_Empty;
		return false;
	}

	*key   = String_UNSAFE_Substring(str, 0, idx); idx++;
	*value = String_UNSAFE_SubstringAt(str, idx);

	/* Trim key [c] value to just key[c]value */
	String_UNSAFE_TrimEnd(key);
	String_UNSAFE_TrimStart(value);
	return key->length > 0 && value->length > 0;
}


int String_Equals(const cc_string* a, const cc_string* b) {
	return a->length == b->length && Mem_Equal(a->buffer, b->buffer, a->length);
} 

int String_CaselessEquals(const cc_string* a, const cc_string* b) {
	int i;
	char aCur, bCur;
	if (a->length != b->length) return false;

	for (i = 0; i < a->length; i++) {
		aCur = a->buffer[i]; Char_MakeLower(aCur);
		bCur = b->buffer[i]; Char_MakeLower(bCur);
		if (aCur != bCur) return false;
	}
	return true;
}

int String_CaselessEqualsConst(const cc_string* a, const char* b) {
	int i;
	char aCur, bCur;

	for (i = 0; i < a->length; i++) {
		aCur = a->buffer[i]; Char_MakeLower(aCur);
		bCur = b[i];         Char_MakeLower(bCur);
		if (aCur != bCur || bCur == '\0') return false;
	}
	/* ensure at end of string */
	return b[a->length] == '\0';
}


void String_Append(cc_string* str, char c) {
	if (str->length == str->capacity) return;
	str->buffer[str->length++] = c;
}

void String_AppendBool(cc_string* str, cc_bool value) {
	String_AppendConst(str, value ? "True" : "False");
}

int String_MakeUInt32(cc_uint32 num, char* digits) {
	int len = 0;
	do {
		digits[len] = '0' + (num % 10); num /= 10; len++;
	} while (num > 0);
	return len;
}

void String_AppendInt(cc_string* str, int num) {
	if (num < 0) {
		num = -num;
		String_Append(str, '-');
	}
	String_AppendUInt32(str, (cc_uint32)num);
}

void String_AppendUInt32(cc_string* str, cc_uint32 num) {
	char digits[STRING_INT_CHARS];
	int i, count = String_MakeUInt32(num, digits);

	for (i = count - 1; i >= 0; i--) {
		String_Append(str, digits[i]);
	}
}

void String_AppendPaddedInt(cc_string* str, int num, int minDigits) {
	char digits[STRING_INT_CHARS];
	int i, count;
	for (i = 0; i < minDigits; i++) { digits[i] = '0'; }

	count = String_MakeUInt32(num, digits);
	if (count < minDigits) count = minDigits;

	for (i = count - 1; i >= 0; i--) {
		String_Append(str, digits[i]);
	}
}

void String_AppendFloat(cc_string* str, float num, int fracDigits) {
	int i, whole, digit;
	double frac;

	if (num < 0.0f) {
		String_Append(str, '-'); /* don't need to check success */
		num = -num;
	}

	whole = (int)num;
	String_AppendUInt32(str, whole);

	frac = (double)num - (double)whole;
	if (frac == 0.0) return;
	String_Append(str, '.'); /* don't need to check success */

	for (i = 0; i < fracDigits; i++) {
		frac *= 10;
		digit = (int)frac % 10;
		String_Append(str, '0' + digit);
	}
}

void String_AppendHex(cc_string* str, cc_uint8 value) {
	/* 48 = index of 0, 55 = index of (A - 10) */
	cc_uint8 hi = (value >> 4) & 0xF;
	char c_hi  = hi < 10 ? (hi + 48) : (hi + 55);
	cc_uint8 lo = value & 0xF;
	char c_lo  = lo < 10 ? (lo + 48) : (lo + 55);

	String_Append(str, c_hi);
	String_Append(str, c_lo);
}

CC_NOINLINE static void String_Hex32(cc_string* str, cc_uint32 value) {
	int shift;

	for (shift = 24; shift >= 0; shift -= 8) {
		cc_uint8 part = (cc_uint8)(value >> shift);
		String_AppendHex(str, part);
	}
}

CC_NOINLINE static void String_Hex64(cc_string* str, cc_uint64 value) {
	int shift;

	for (shift = 56; shift >= 0; shift -= 8) {
		cc_uint8 part = (cc_uint8)(value >> shift);
		String_AppendHex(str, part);
	}
}

void String_AppendConst(cc_string* str, const char* src) {
	for (; *src; src++) {
		String_Append(str, *src);
	}
}

void String_AppendAll(cc_string* str, const void* data, int len) {
	const char* src = (const char*)data;
	int i;
	for (i = 0; i < len; i++) String_Append(str, src[i]);
}

void String_AppendString(cc_string* str, const cc_string* src) {
	int i;
	for (i = 0; i < src->length; i++) {
		String_Append(str, src->buffer[i]);
	}
}

void String_AppendColorless(cc_string* str, const cc_string* src) {
	char c;
	int i;

	for (i = 0; i < src->length; i++) {
		c = src->buffer[i];
		if (c == '&') { i++; continue; } /* Skip over the following color code */
		String_Append(str, c);
	}
}


int String_IndexOfAt(const cc_string* str, int offset, char c) {
	int i;
	for (i = offset; i < str->length; i++) {
		if (str->buffer[i] == c) return i;
	}
	return -1;
}

int String_LastIndexOfAt(const cc_string* str, int offset, char c) {
	int i;
	for (i = (str->length - 1) - offset; i >= 0; i--) {
		if (str->buffer[i] == c) return i;
	}
	return -1;
}

void String_InsertAt(cc_string* str, int offset, char c) {
	int i;

	if (offset < 0 || offset > str->length) {
		Logger_Abort("Offset for InsertAt out of range");
	}
	if (str->length == str->capacity) {
		Logger_Abort("Cannot insert character into full string");
	}
	
	for (i = str->length; i > offset; i--) {
		str->buffer[i] = str->buffer[i - 1];
	}
	str->buffer[offset] = c;
	str->length++;
}

void String_DeleteAt(cc_string* str, int offset) {
	int i;

	if (offset < 0 || offset >= str->length) {
		Logger_Abort("Offset for DeleteAt out of range");
	}
	
	for (i = offset; i < str->length - 1; i++) {
		str->buffer[i] = str->buffer[i + 1];
	}
	str->buffer[str->length - 1] = '\0';
	str->length--;
}

void String_UNSAFE_TrimStart(cc_string* str) {
	int i;
	for (i = 0; i < str->length; i++) {
		if (str->buffer[i] != ' ') break;
			
		str->buffer++;
		str->length--; i--;
	}
}

void String_UNSAFE_TrimEnd(cc_string* str) {
	int i;
	for (i = str->length - 1; i >= 0; i--) {
		if (str->buffer[i] != ' ') break;
		str->length--;
	}
}

int String_IndexOfConst(const cc_string* str, const char* sub) {
	int i, j;

	for (i = 0; i < str->length; i++) {
		for (j = 0; sub[j] && (i + j) < str->length; j++) {

			if (str->buffer[i + j] != sub[j]) break;
		}
		if (sub[j] == '\0') return i;
	}
	return -1;
}

int String_CaselessContains(const cc_string* str, const cc_string* sub) {
	char strCur, subCur;
	int i, j;

	for (i = 0; i < str->length; i++) {
		for (j = 0; j < sub->length && (i + j) < str->length; j++) {

			strCur = str->buffer[i + j]; Char_MakeLower(strCur);
			subCur = sub->buffer[j];     Char_MakeLower(subCur);
			if (strCur != subCur) break;
		}
		if (j == sub->length) return true;
	}
	return false;
}

int String_CaselessStarts(const cc_string* str, const cc_string* sub) {
	char strCur, subCur;
	int i;
	if (str->length < sub->length) return false;

	for (i = 0; i < sub->length; i++) {
		strCur = str->buffer[i]; Char_MakeLower(strCur);
		subCur = sub->buffer[i]; Char_MakeLower(subCur);
		if (strCur != subCur) return false;
	}
	return true;
}

int String_CaselessEnds(const cc_string* str, const cc_string* sub) {
	char strCur, subCur;
	int i, j = str->length - sub->length;	
	if (j < 0) return false; /* sub longer than str */

	for (i = 0; i < sub->length; i++) {
		strCur = str->buffer[j + i]; Char_MakeLower(strCur);
		subCur = sub->buffer[i];     Char_MakeLower(subCur);
		if (strCur != subCur) return false;
	}
	return true;
}

int String_Compare(const cc_string* a, const cc_string* b) {
	char aCur, bCur;
	int i, minLen = min(a->length, b->length);

	for (i = 0; i < minLen; i++) {
		aCur = a->buffer[i]; Char_MakeLower(aCur);
		bCur = b->buffer[i]; Char_MakeLower(bCur);

		if (aCur == bCur) continue;
		return (cc_uint8)aCur - (cc_uint8)bCur;
	}

	/* all chars are equal here - same string, or a substring */
	return a->length - b->length;
}

void String_Format1(cc_string* str, const char* format, const void* a1) {
	String_Format4(str, format, a1, NULL, NULL, NULL);
}
void String_Format2(cc_string* str, const char* format, const void* a1, const void* a2) {
	String_Format4(str, format, a1, a2, NULL, NULL);
}
void String_Format3(cc_string* str, const char* format, const void* a1, const void* a2, const void* a3) {
	String_Format4(str, format, a1, a2, a3, NULL);
}
void String_Format4(cc_string* str, const char* format, const void* a1, const void* a2, const void* a3, const void* a4) {
	const void* arg;
	int i, j = 0, digits;

	const void* args[4];
	args[0] = a1; args[1] = a2; args[2] = a3; args[3] = a4;

	for (i = 0; format[i]; i++) {
		if (format[i] != '%') { String_Append(str, format[i]); continue; }
		arg = args[j++];

		switch (format[++i]) {
		case 'b': 
			String_AppendInt(str, *((cc_uint8*)arg)); break;
		case 'i': 
			String_AppendInt(str, *((int*)arg)); break;
		case 'f': 
			digits = format[++i] - '0';
			String_AppendFloat(str, *((float*)arg), digits); break;
		case 'p':
			digits = format[++i] - '0';
			String_AppendPaddedInt(str, *((int*)arg), digits); break;
		case 't': 
			String_AppendBool(str, *((cc_bool*)arg)); break;
		case 'c': 
			String_AppendConst(str, (char*)arg);  break;
		case 's': 
			String_AppendString(str, (cc_string*)arg);  break;
		case 'r':
			String_Append(str, *((char*)arg)); break;
		case 'x':
			if (sizeof(cc_uintptr) == 4) {
				String_Hex32(str, *((cc_uint32*)arg)); break;
			} else {
				String_Hex64(str, *((cc_uint64*)arg)); break;
			}
		case 'h':
			String_Hex32(str, *((cc_uint32*)arg)); break;
		case '%':
			String_Append(str, '%'); break;
		default: 
			Logger_Abort("Invalid type for string format");
		}
	}
}


/*########################################################################################################################*
*------------------------------------------------Character set conversions------------------------------------------------*
*#########################################################################################################################*/
static const cc_unichar controlChars[32] = {
	0x0000, 0x263A, 0x263B, 0x2665, 0x2666, 0x2663, 0x2660, 0x2022,
	0x25D8, 0x25CB, 0x25D9, 0x2642, 0x2640, 0x266A, 0x266B, 0x263C,
	0x25BA, 0x25C4, 0x2195, 0x203C, 0x00B6, 0x00A7, 0x25AC, 0x21A8,
	0x2191, 0x2193, 0x2192, 0x2190, 0x221F, 0x2194, 0x25B2, 0x25BC
};

static const cc_unichar extendedChars[129] = { 0x2302,
	0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7,
	0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
	0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9,
	0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192,
	0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA,
	0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,
	0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,
	0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
	0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F,
	0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
	0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B,
	0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
	0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4,
	0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229,
	0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248,
	0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, 0x00A0
};

cc_unichar Convert_CP437ToUnicode(char c) {
	cc_uint8 raw = (cc_uint8)c;
	if (raw < 0x20) return controlChars[raw];
	if (raw < 0x7F) return raw;
	return extendedChars[raw - 0x7F];
}

char Convert_CodepointToCP437(cc_codepoint cp) {
	char c; Convert_TryCodepointToCP437(cp, &c); return c;
}

static cc_codepoint ReduceEmoji(cc_codepoint cp) {
	if (cp == 0x1F31E) return 0x263C;
	if (cp == 0x1F3B5) return 0x266B;
	if (cp == 0x1F642) return 0x263A;

	if (cp == 0x1F600 || cp == 0x1F601 || cp == 0x1F603) return 0x263A;
	if (cp == 0x1F604 || cp == 0x1F606 || cp == 0x1F60A) return 0x263A;
	return cp;
}

cc_bool Convert_TryCodepointToCP437(cc_codepoint cp, char* c) {
	int i;
	if (cp >= 0x20 && cp < 0x7F) { *c = (char)cp; return true; }
	if (cp >= 0x1F000) cp = ReduceEmoji(cp);

	for (i = 0; i < Array_Elems(controlChars); i++) {
		if (controlChars[i] == cp) { *c = i; return true; }
	}
	for (i = 0; i < Array_Elems(extendedChars); i++) {
		if (extendedChars[i] == cp) { *c = i + 0x7F; return true; }
	}

	*c = '?'; return false;
}

int Convert_Utf8ToCodepoint(cc_codepoint* cp, const cc_uint8* data, cc_uint32 len) {
	*cp = '\0';
	if (!len) return 0;

	if (data[0] <= 0x7F) {
		*cp = data[0];
		return 1;
	} else if ((data[0] & 0xE0) == 0xC0) {
		if (len < 2) return 0;

		*cp = ((data[0] & 0x1F) << 6)  | ((data[1] & 0x3F));
		return 2;
	} else if ((data[0] & 0xF0) == 0xE0) {
		if (len < 3) return 0;

		*cp = ((data[0] & 0x0F) << 12) | ((data[1] & 0x3F) << 6) 
			| ((data[2] & 0x3F));
		return 3;
	} else {
		if (len < 4) return 0;

		*cp = ((data[0] & 0x07) << 18) | ((data[1] & 0x3F) << 12) 
			| ((data[2] & 0x3F) << 6)  |  (data[3] & 0x3F);
		return 4;
	}
}

/* Encodes a unicode character in UTF8, returning number of bytes written */
static int Convert_UnicodeToUtf8(cc_unichar uc, cc_uint8* data) {
	if (uc <= 0x7F) {
		data[0] = (cc_uint8)uc;
		return 1;
	} else if (uc <= 0x7FF) {
		data[0] = 0xC0 | ((uc >> 6) & 0x1F);
		data[1] = 0x80 | ((uc)      & 0x3F);
		return 2;
	} else {
		data[0] = 0xE0 | ((uc >> 12) & 0x0F);
		data[1] = 0x80 | ((uc >>  6) & 0x3F);
		data[2] = 0x80 | ((uc)       & 0x3F);
		return 3;
	}
}

int Convert_CP437ToUtf8(char c, cc_uint8* data) {
	/* Common ASCII case */
	if (c >= 0x20 && c < 0x7F) {
		data[0] = (cc_uint8)c;
		return 1;
	}
	return Convert_UnicodeToUtf8(Convert_CP437ToUnicode(c), data);
}

void String_AppendUtf16(cc_string* value, const void* data, int numBytes) {
	const cc_unichar* chars = (const cc_unichar*)data;
	int i; char c;
	
	for (i = 0; i < (numBytes >> 1); i++) {
		/* TODO: UTF16 to codepoint conversion */
		if (Convert_TryCodepointToCP437(chars[i], &c)) String_Append(value, c);
	}
}

void String_AppendUtf8(cc_string* value, const void* data, int numBytes) {
	const cc_uint8* chars = (const cc_uint8*)data;
	int len; cc_codepoint cp; char c;

	for (; numBytes > 0; numBytes -= len) {
		len = Convert_Utf8ToCodepoint(&cp, chars, numBytes);
		if (!len) return;

		if (Convert_TryCodepointToCP437(cp, &c)) String_Append(value, c);
		chars += len;
	}
}

void String_DecodeCP1252(cc_string* value, const void* data, int numBytes) {
	const cc_uint8* chars = (const cc_uint8*)data;
	int i; char c;

	for (i = 0; i < numBytes; i++) {
		if (Convert_TryCodepointToCP437(chars[i], &c)) String_Append(value, c);
	}
}

int String_EncodeUtf8(void* data, const cc_string* src) {
	cc_uint8* dst = (cc_uint8*)data;
	cc_uint8* cur;
	int i, len = 0;
	if (src->length > FILENAME_SIZE) Logger_Abort("String too long to expand");

	for (i = 0; i < src->length; i++) {
		cur = dst + len;
		len += Convert_CP437ToUtf8(src->buffer[i], cur);
	}
	dst[len] = '\0';
	return len;
}


/*########################################################################################################################*
*--------------------------------------------------Numerical conversions--------------------------------------------------*
*#########################################################################################################################*/
cc_bool Convert_ParseUInt8(const cc_string* str, cc_uint8* value) {
	int tmp; 
	*value = 0;
	if (!Convert_ParseInt(str, &tmp) || tmp < 0 || tmp > UInt8_MaxValue) return false;
	*value = (cc_uint8)tmp; return true;
}

cc_bool Convert_ParseUInt16(const cc_string* str, cc_uint16* value) {
	int tmp; 
	*value = 0;
	if (!Convert_ParseInt(str, &tmp) || tmp < 0 || tmp > UInt16_MaxValue) return false;
	*value = (cc_uint16)tmp; return true;
}

static int Convert_CompareDigits(const char* digits, const char* magnitude) {
	int i;
	for (i = 0; ; i++) {
		if (magnitude[i] == '\0')     return  0;
		if (digits[i] > magnitude[i]) return  1;
		if (digits[i] < magnitude[i]) return -1;
	}
	return 0;
}

static cc_bool Convert_TryParseDigits(const cc_string* str, cc_bool* negative, char* digits, int maxDigits) {
	char* start = digits;
	int offset = 0, i;

	*negative = false;
	if (!str->length) return false;
	digits += (maxDigits - 1);

	/* Handle number signs */
	if (str->buffer[0] == '-') { *negative = true; offset = 1; }
	if (str->buffer[0] == '+') { offset = 1; }

	/* add digits, starting at last digit */
	for (i = str->length - 1; i >= offset; i--) {
		char c = str->buffer[i];
		if (c < '0' || c > '9' || digits < start) return false;
		*digits-- = c;
	}

	for (; digits >= start; ) { *digits-- = '0'; }
	return true;
}

#define INT32_DIGITS 10
cc_bool Convert_ParseInt(const cc_string* str, int* value) {
	cc_bool negative;
	char digits[INT32_DIGITS];
	int i, compare, sum = 0;

	*value = 0;	
	if (!Convert_TryParseDigits(str, &negative, digits, INT32_DIGITS)) return false;
	
	if (negative) {
		compare = Convert_CompareDigits(digits, "2147483648");
		/* Special case, since |largest min value| is > |largest max value| */
		if (compare == 0) { *value = Int32_MinValue; return true; }
	} else {
		compare = Convert_CompareDigits(digits, "2147483647");
	}
	if (compare > 0) return false;

	for (i = 0; i < INT32_DIGITS; i++) {
		sum *= 10; sum += digits[i] - '0';
	}

	if (negative) sum = -sum;
	*value = sum;
	return true;
}

#define UINT64_DIGITS 20
cc_bool Convert_ParseUInt64(const cc_string* str, cc_uint64* value) {
	cc_bool negative;
	char digits[UINT64_DIGITS];
	int i, compare;
	cc_uint64 sum = 0;

	*value = 0;
	if (!Convert_TryParseDigits(str, &negative, digits, UINT64_DIGITS)) return false;

	compare = Convert_CompareDigits(digits, "18446744073709551615");
	if (negative || compare > 0) return false;

	for (i = 0; i < UINT64_DIGITS; i++) {
		sum *= 10; sum += digits[i] - '0';
	}

	*value = sum;
	return true;
}

cc_bool Convert_ParseFloat(const cc_string* str, float* value) {
	int i = 0;
	cc_bool negate = false;
	double sum, whole, fract, divide = 10.0;
	*value = 0.0f;

	/* Handle number signs */
	if (!str->length) return false;
	if (str->buffer[0] == '-') { negate = true; i++; }
	if (str->buffer[0] == '+') { i++; }

	for (whole = 0.0; i < str->length; i++) {
		char c = str->buffer[i];
		if (c == '.' || c == ',') { i++; break; }

		if (c < '0' || c > '9') return false;
		whole *= 10; whole += (c - '0');
	}

	for (fract = 0.0; i < str->length; i++) {
		char c = str->buffer[i];
		if (c < '0' || c > '9') return false;

		fract += (c - '0') / divide; divide *= 10;
	}

	sum = whole + fract;
	if (negate) sum = -sum;
	*value = (float)sum;
	return true;
}

cc_bool Convert_ParseBool(const cc_string* str, cc_bool* value) {
	if (String_CaselessEqualsConst(str, "True")) {
		*value = true; return true;
	}
	if (String_CaselessEqualsConst(str, "False")) {
		*value = false; return true;
	}
	*value = false; return false;
}


/*########################################################################################################################*
*------------------------------------------------------StringsBuffer------------------------------------------------------*
*#########################################################################################################################*/
#define STRINGSBUFFER_BUFFER_EXPAND_SIZE 8192

#define StringsBuffer_GetOffset(raw)  ((raw) >> buffer->_lenShift)
#define StringsBuffer_GetLength(raw)  ((raw)  & buffer->_lenMask)
#define StringsBuffer_PackOffset(off) ((off) << buffer->_lenShift)

void StringsBuffer_Init(struct StringsBuffer* buffer) {
	buffer->count       = 0;
	buffer->totalLength = 0;
	buffer->textBuffer     = buffer->_defaultBuffer;
	buffer->flagsBuffer    = buffer->_defaultFlags;
	buffer->_textCapacity  = STRINGSBUFFER_BUFFER_DEF_SIZE;
	buffer->_flagsCapacity = STRINGSBUFFER_FLAGS_DEF_ELEMS;

	if (buffer->_lenShift) return;
	StringsBuffer_SetLengthBits(buffer, STRINGSBUFFER_DEF_LEN_SHIFT);
}

void StringsBuffer_SetLengthBits(struct StringsBuffer* buffer, int bits) {
	buffer->_lenShift = bits;
	buffer->_lenMask  = (1 << bits) - 1;
}

void StringsBuffer_Clear(struct StringsBuffer* buffer) {
	/* Never initialised to begin with */
	if (!buffer->_flagsCapacity) return;

	if (buffer->textBuffer != buffer->_defaultBuffer) {
		Mem_Free(buffer->textBuffer);
	}
	if (buffer->flagsBuffer != buffer->_defaultFlags) {
		Mem_Free(buffer->flagsBuffer);
	}
	StringsBuffer_Init(buffer);
}

cc_string StringsBuffer_UNSAFE_Get(struct StringsBuffer* buffer, int i) {
	cc_uint32 flags, offset, len;
	if (i < 0 || i >= buffer->count) Logger_Abort("Tried to get String past StringsBuffer end");

	flags  = buffer->flagsBuffer[i];
	offset = StringsBuffer_GetOffset(flags);
	len    = StringsBuffer_GetLength(flags);
	return String_Init(&buffer->textBuffer[offset], len, len);
}

void StringsBuffer_UNSAFE_GetRaw(struct StringsBuffer* buffer, int i, cc_string* dst) {
	cc_uint32 flags = buffer->flagsBuffer[i];
	dst->buffer     = buffer->textBuffer + StringsBuffer_GetOffset(flags);
	dst->length     = StringsBuffer_GetLength(flags);
	dst->capacity   = 0;
}

void StringsBuffer_Add(struct StringsBuffer* buffer, const cc_string* str) {
	int textOffset;
	/* StringsBuffer hasn't been initialised yet, do it here */
	if (!buffer->_flagsCapacity) StringsBuffer_Init(buffer);

	if (buffer->count == buffer->_flagsCapacity) {
		Utils_Resize((void**)&buffer->flagsBuffer, &buffer->_flagsCapacity,
					4, STRINGSBUFFER_FLAGS_DEF_ELEMS, 512);
	}

	if (str->length > buffer->_lenMask) {
		Logger_Abort("String too big to insert into StringsBuffer");
	}

	textOffset = buffer->totalLength;
	if (textOffset + str->length >= buffer->_textCapacity) {
		Utils_Resize((void**)&buffer->textBuffer, &buffer->_textCapacity,
					1, STRINGSBUFFER_BUFFER_DEF_SIZE, 8192);
	}

	Mem_Copy(&buffer->textBuffer[textOffset], str->buffer, str->length);
	buffer->flagsBuffer[buffer->count] = str->length | StringsBuffer_PackOffset(textOffset);

	buffer->count++;
	buffer->totalLength += str->length;
}

void StringsBuffer_Remove(struct StringsBuffer* buffer, int index) {
	cc_uint32 flags, offset, len;
	cc_uint32 i, offsetAdj;
	if (index < 0 || index >= buffer->count) Logger_Abort("Tried to remove String past StringsBuffer end");

	flags  = buffer->flagsBuffer[index];
	offset = StringsBuffer_GetOffset(flags);
	len    = StringsBuffer_GetLength(flags);

	/* Imagine buffer is this: AAXXYYZZ, and want to delete XX */
	/* We iterate from first char of Y to last char of Z, */
	/* shifting each character two to the left. */
	for (i = offset + len; i < buffer->totalLength; i++) {
		buffer->textBuffer[i - len] = buffer->textBuffer[i]; 
	}

	/* Adjust text offset of elements after this element */
	/* Elements may not be in order so must account for that */
	offsetAdj = StringsBuffer_PackOffset(len);
	for (i = index; i < buffer->count - 1; i++) {
		buffer->flagsBuffer[i] = buffer->flagsBuffer[i + 1];
		if (buffer->flagsBuffer[i] >= flags) {
			buffer->flagsBuffer[i] -= offsetAdj;
		}
	}
	
	buffer->count--;
	buffer->totalLength -= len;
}

static struct StringsBuffer* sort_buffer;
static void StringsBuffer_QuickSort(int left, int right) {
	struct StringsBuffer* buffer = sort_buffer;
	cc_uint32* keys = buffer->flagsBuffer; cc_uint32 key;

	while (left < right) {
		int i = left, j = right;
		cc_string pivot = StringsBuffer_UNSAFE_Get(buffer, (i + j) >> 1);
		cc_string strI, strJ;

		/* partition the list */
		while (i <= j) {
			while ((strI = StringsBuffer_UNSAFE_Get(buffer, i), String_Compare(&pivot, &strI)) > 0) i++;
			while ((strJ = StringsBuffer_UNSAFE_Get(buffer, j), String_Compare(&pivot, &strJ)) < 0) j--;
			QuickSort_Swap_Maybe();
		}
		/* recurse into the smaller subset */
		QuickSort_Recurse(StringsBuffer_QuickSort)
	}
}

void StringsBuffer_Sort(struct StringsBuffer* buffer) {
	sort_buffer = buffer;
	StringsBuffer_QuickSort(0, buffer->count - 1);
}


/*########################################################################################################################*
*------------------------------------------------------Word wrapper-------------------------------------------------------*
*#########################################################################################################################*/
static cc_bool WordWrap_IsWrapper(char c) {
	return c == '\0' || c == ' ' || c == '-' || c == '>' || c == '<' || c == '/' || c == '\\';
}

void WordWrap_Do(STRING_REF cc_string* text, cc_string* lines, int numLines, int lineLen) {
	int i, lineStart, lineEnd;
	for (i = 0; i < numLines; i++) { lines[i] = String_Empty; }

	for (i = 0, lineStart = 0; i < numLines; i++) {
		int nextLineStart = lineStart + lineLen;
		/* No more text to wrap */
		if (nextLineStart >= text->length) {
			lines[i] = String_UNSAFE_SubstringAt(text, lineStart); return;
		}

		/* Find beginning of last word on current line */
		for (lineEnd = nextLineStart; lineEnd >= lineStart; lineEnd--) {
			if (WordWrap_IsWrapper(text->buffer[lineEnd])) break;
		}
		lineEnd++; /* move after wrapper char (i.e. beginning of last word) */

		if (lineEnd <= lineStart || lineEnd >= nextLineStart) {
			/* Three special cases handled by this: */
			/* - Entire line is filled with a single word */
			/* - Last character(s) on current line are wrapper characters */
			/* - First character on next line is a wrapper character (last word ends at current line end) */
			lines[i] = String_UNSAFE_Substring(text, lineStart, lineLen);
			lineStart += lineLen;
		} else {
			/* Last word in current line does not end in current line (extends onto next line) */
			/* Trim current line to end at beginning of last word */
			/* Set next line to start at beginning of last word */
			lines[i] = String_UNSAFE_Substring(text, lineStart, lineEnd - lineStart);
			lineStart = lineEnd;
		}
	}
}

void WordWrap_GetCoords(int index, const cc_string* lines, int numLines, int* coordX, int* coordY) {
	int y, offset = 0, lineLen;
	if (index == -1) index = Int32_MaxValue;
	*coordX = -1; *coordY = 0;

	for (y = 0; y < numLines; y++) {
		lineLen = lines[y].length;
		if (!lineLen) break;

		*coordY = y;
		if (index < offset + lineLen) {
			*coordX = index - offset; break;
		}
		offset += lineLen;
	}
	if (*coordX == -1) *coordX = lines[*coordY].length;
}

int WordWrap_GetBackLength(const cc_string* text, int index) {
	int start = index;
	if (index <= 0) return 0;
	if (index >= text->length) Logger_Abort("WordWrap_GetBackLength - index past end of string");
	
	/* Go backward to the end of the previous word */
	while (index > 0 && text->buffer[index] == ' ') index--;
	/* Go backward to the start of the current word */
	while (index > 0 && text->buffer[index] != ' ') index--;

	return start - index;
}

int WordWrap_GetForwardLength(const cc_string* text, int index) {
	int start = index, length = text->length;
	if (index == -1) return 0;
	if (index >= text->length) Logger_Abort("WordWrap_GetForwardLength - index past end of string");

	/* Go forward to the end of the word 'index' is currently in */
	while (index < length && text->buffer[index] != ' ') index++;
	/* Go forward to the start of the next word after 'index' */
	while (index < length && text->buffer[index] == ' ') index++;

	return index - start;
}