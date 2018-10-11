#include "String.h"
#include "Funcs.h"
#include "ErrorHandler.h"
#include "Platform.h"
#include "ExtMath.h"
#include "Stream.h"

String String_Init(STRING_REF char* buffer, uint16_t length, uint16_t capacity) {
	String str = { buffer, length, capacity }; return str;
}

String String_InitAndClear(STRING_REF char* buffer, uint16_t capacity) {
	String str = String_Init(buffer, 0, capacity);	
	int i;
	for (i = 0; i < capacity; i++) { buffer[i] = '\0'; }
	return str;
}

uint16_t String_CalcLen(const char* raw, uint16_t capacity) {
	uint16_t length = 0;
	while (length < capacity && *raw) { raw++; length++; }
	return length;
}

String String_MakeNull(void) { return String_Init(NULL, 0, 0); }

String String_FromRaw(STRING_REF char* buffer, uint16_t capacity) {
	return String_Init(buffer, String_CalcLen(buffer, capacity), capacity);
}

String String_FromReadonly(STRING_REF const char* buffer) {
	uint16_t len = String_CalcLen(buffer, UInt16_MaxValue);
	return String_Init(buffer, len, len);
}


void String_StripCols(String* str) {
	int i;
	for (i = str->length - 1; i >= 0; i--) {
		if (str->buffer[i] != '&') continue;
		/* Remove the & and the colour code following it */
		String_DeleteAt(str, i); String_DeleteAt(str, i);
	}
}

void String_Copy(String* dst, const String* src) {
	dst->length = 0;
	String_AppendString(dst, src);
}

String String_UNSAFE_Substring(STRING_REF const String* str, int offset, int length) {
	if (offset < 0 || offset > str->length) {
		ErrorHandler_Fail("Offset for substring out of range");
	}
	if (length < 0 || length > str->length) {
		ErrorHandler_Fail("Length for substring out of range");
	}
	if (offset + length > str->length) {
		ErrorHandler_Fail("Result substring is out of range");
	}
	return String_Init(str->buffer + offset, length, length);
}

void String_UNSAFE_Split(STRING_REF const String* str, char c, String* subs, int* subsCount) {
	int maxSubs = *subsCount, i = 0, start = 0;
	for (; i < maxSubs && start <= str->length; i++) {
		int end = String_IndexOf(str, c, start);
		if (end == -1) end = str->length;

		subs[i] = String_UNSAFE_Substring(str, start, end - start);
		start = end + 1;
	}

	*subsCount = i;
	/* If not enough split substrings, make remaining null */
	for (; i < maxSubs; i++) { subs[i] = String_MakeNull(); }
}

bool String_UNSAFE_Separate(STRING_REF const String* str, char c, String* key, String* value) {
	/* key [c] value or key[c]value */
	int idx = String_IndexOf(str, c, 0);
	if (idx <= 0) return false;                 /* missing [c] or no key */
	if ((idx + 1) >= str->length) return false; /* missing value */

	*key   = String_UNSAFE_Substring(str, 0, idx); idx++;
	*value = String_UNSAFE_SubstringAt(str, idx);

	String_TrimEnd(key);
	String_TrimStart(value);
	return true;
}


bool String_Equals(const String* a, const String* b) {
	if (a->length != b->length) return false;
	int i;

	for (i = 0; i < a->length; i++) {
		if (a->buffer[i] != b->buffer[i]) return false;
	}
	return true;
}

bool String_CaselessEquals(const String* a, const String* b) {
	if (a->length != b->length) return false;
	int i;

	for (i = 0; i < a->length; i++) {
		char aCur = a->buffer[i]; Char_MakeLower(aCur);
		char bCur = b->buffer[i]; Char_MakeLower(bCur);
		if (aCur != bCur) return false;
	}
	return true;
}

bool String_CaselessEqualsConst(const String* a, const char* b) {
	int i;

	for (i = 0; i < a->length; i++) {
		char aCur = a->buffer[i]; Char_MakeLower(aCur);
		char bCur = b[i];         Char_MakeLower(bCur);
		if (aCur != bCur || bCur == '\0') return false;
	}
	/* ensure at end of string */
	return b[a->length] == '\0';
}


bool String_Append(String* str, char c) {
	if (str->length == str->capacity) return false;

	str->buffer[str->length] = c;
	str->length++;
	return true;
}

bool String_AppendBool(String* str, bool value) {
	const char* text = value ? "True" : "False";
	return String_AppendConst(str, text);
}

int String_MakeUInt32(uint32_t num, char* numBuffer) {
	int len = 0;
	do {
		numBuffer[len] = '0' + (num % 10); num /= 10; len++;
	} while (num > 0);
	return len;
}

bool String_AppendInt(String* str, int num) {
	if (num < 0) {
		num = -num;
		if (!String_Append(str, '-')) return false;
	}
	return String_AppendUInt32(str, (uint32_t)num);
}

bool String_AppendUInt32(String* str, uint32_t num) {
	char numBuffer[STRING_INT_CHARS];
	int i, numLen = String_MakeUInt32(num, numBuffer);

	for (i = numLen - 1; i >= 0; i--) {
		if (!String_Append(str, numBuffer[i])) return false;
	}
	return true;
}

/* Attempts to append an integer value to the end of a string, padding left with 0. */
bool String_AppendPaddedInt(String* str, int num, int minDigits) {
	char numBuffer[STRING_INT_CHARS];
	int i;
	for (i = 0; i < minDigits; i++) { numBuffer[i] = '0'; }

	int numLen = String_MakeUInt32(num, numBuffer);
	if (numLen < minDigits) numLen = minDigits;

	for (i = numLen - 1; i >= 0; i--) {
		if (!String_Append(str, numBuffer[i])) return false;
	}
	return true;
}

int String_MakeUInt64(uint64_t num, char* numBuffer) {
	int len = 0;
	do {
		numBuffer[len] = '0' + (num % 10); num /= 10; len++;
	} while (num > 0);
	return len;
}

bool String_AppendUInt64(String* str, uint64_t num) {
	char numBuffer[STRING_INT_CHARS];
	int i, numLen = String_MakeUInt64(num, numBuffer);

	for (i = numLen - 1; i >= 0; i--) {
		if (!String_Append(str, numBuffer[i])) return false;
	}
	return true;
}

bool String_AppendFloat(String* str, float num, int fracDigits) {
	if (num < 0.0f) {
		if (!String_Append(str, '-')) return false;
		num = -num;
	}

	int wholePortion = (int)num;
	if (!String_AppendUInt32(str, wholePortion)) return false;

	double frac = (double)num - (double)wholePortion;
	if (frac == 0.0) return true;
	if (!String_Append(str, '.')) return false;

	int i;
	for (i = 0; i < fracDigits; i++) {
		frac *= 10;
		int digit = Math_AbsI((int)frac) % 10;
		if (!String_Append(str, '0' + digit)) return false;
	}
	return true;
}

bool String_AppendHex(String* str, uint8_t value) {
	/* 48 = index of 0, 55 = index of (A - 10) */
	uint8_t hi = (value >> 4) & 0xF;
	char c_hi  = hi < 10 ? (hi + 48) : (hi + 55);
	uint8_t lo = value & 0xF;
	char c_lo  = lo < 10 ? (lo + 48) : (lo + 55);

	return String_Append(str, c_hi) && String_Append(str, c_lo);
}

NOINLINE_ static bool String_Hex32(String* str, uint32_t value) {
	bool appended;
	int shift;

	for (shift = 24; shift >= 0; shift -= 8) {
		uint8_t part = (uint8_t)(value >> shift);
		appended = String_AppendHex(str, part);
	}
	return appended;
}

NOINLINE_ static bool String_Hex64(String* str, uint64_t value) {
	bool appended;
	int shift;

	for (shift = 56; shift >= 0; shift -= 8) {
		uint8_t part = (uint8_t)(value >> shift);
		appended = String_AppendHex(str, part);
	}
	return appended;
}

bool String_AppendConst(String* str, const char* src) {
	while (*src) {
		if (!String_Append(str, *src)) return false;
		src++;
	}
	return true;
}

bool String_AppendString(String* str, const String* src) {
	int i;

	for (i = 0; i < src->length; i++) {
		if (!String_Append(str, src->buffer[i])) return false;
	}
	return true;
}

bool String_AppendColorless(String* str, const String* src) {
	int i;

	for (i = 0; i < src->length; i++) {
		char c = src->buffer[i];
		if (c == '&') { i++; continue; } /* Skip over the following colour code */
		if (!String_Append(str, c)) return false;
	}
	return true;
}


int String_IndexOf(const String* str, char c, int offset) {
	int i;
	for (i = offset; i < str->length; i++) {
		if (str->buffer[i] == c) return i;
	}
	return -1;
}

int String_LastIndexOf(const String* str, char c) {
	int i;
	for (i = str->length - 1; i >= 0; i--) {
		if (str->buffer[i] == c) return i;
	}
	return -1;
}

void String_InsertAt(String* str, int offset, char c) {
	if (offset < 0 || offset > str->length) {
		ErrorHandler_Fail("Offset for InsertAt out of range");
	}
	if (str->length == str->capacity) {
		ErrorHandler_Fail("Cannot insert character into full string");
	}

	int i;
	for (i = str->length; i > offset; i--) {
		str->buffer[i] = str->buffer[i - 1];
	}

	str->buffer[offset] = c;
	str->length++;
}

void String_DeleteAt(String* str, int offset) {
	if (offset < 0 || offset >= str->length) {
		ErrorHandler_Fail("Offset for DeleteAt out of range");
	}

	int i;
	for (i = offset; i < str->length - 1; i++) {
		str->buffer[i] = str->buffer[i + 1];
	}

	str->buffer[str->length - 1] = '\0';
	str->length--;
}

void String_TrimStart(String* str) {
	int i;
	for (i = 0; i < str->length; i++) {
		if (str->buffer[i] != ' ') break;
			
		str->buffer++;
		str->length--; i--;
	}
}

void String_TrimEnd(String* str) {
	int i;
	for (i = str->length - 1; i >= 0; i--) {
		if (str->buffer[i] != ' ') break;
		str->length--;
	}
}

int String_IndexOfString(const String* str, const String* sub) {
	int i, j;
	for (i = 0; i < str->length; i++) {
		for (j = 0; j < sub->length && (i + j) < str->length; j++) {

			if (str->buffer[i + j] != sub->buffer[j]) break;
		}
		if (j == sub->length) return i;
	}
	return -1;
}

bool String_CaselessContains(const String* str, const String* sub) {
	int i, j;
	for (i = 0; i < str->length; i++) {
		for (j = 0; j < sub->length && (i + j) < str->length; j++) {

			char strCur = str->buffer[i + j]; Char_MakeLower(strCur);
			char subCur = sub->buffer[j];     Char_MakeLower(subCur);
			if (strCur != subCur) break;
		}
		if (j == sub->length) return true;
	}
	return false;
}

bool String_CaselessStarts(const String* str, const String* sub) {
	if (str->length < sub->length) return false;
	int i;

	for (i = 0; i < sub->length; i++) {
		char strCur = str->buffer[i]; Char_MakeLower(strCur);
		char subCur = sub->buffer[i]; Char_MakeLower(subCur);
		if (strCur != subCur) return false;
	}
	return true;
}

bool String_CaselessEnds(const String* str, const String* sub) {
	if (str->length < sub->length) return false;
	int i, j = str->length - sub->length;

	for (i = 0; i < sub->length; i++) {
		char strCur = str->buffer[j + i]; Char_MakeLower(strCur);
		char subCur = sub->buffer[i];     Char_MakeLower(subCur);
		if (strCur != subCur) return false;
	}
	return true;
}

int String_Compare(const String* a, const String* b) {
	int minLen = min(a->length, b->length);
	int i;

	for (i = 0; i < minLen; i++) {
		char aCur = a->buffer[i]; Char_MakeLower(aCur);
		char bCur = b->buffer[i]; Char_MakeLower(bCur);

		if (aCur == bCur) continue;
		return (uint8_t)aCur - (uint8_t)bCur;
	}

	/* all chars are equal here - same string, or a substring */
	return a->length - b->length;
}

void String_Format1(String* str, const char* format, const void* a1) {
	String_Format4(str, format, a1, NULL, NULL, NULL);
}
void String_Format2(String* str, const char* format, const void* a1, const void* a2) {
	String_Format4(str, format, a1, a2, NULL, NULL);
}
void String_Format3(String* str, const char* format, const void* a1, const void* a2, const void* a3) {
	String_Format4(str, format, a1, a2, a3, NULL);
}
void String_Format4(String* str, const char* format, const void* a1, const void* a2, const void* a3, const void* a4) {
	String formatStr = String_FromReadonly(format);
	const void* args[4] = { a1, a2, a3, a4 };
	int i, j = 0, digits;

	for (i = 0; i < formatStr.length; i++) {
		if (formatStr.buffer[i] != '%') { String_Append(str, formatStr.buffer[i]); continue; }

		const void* arg = args[j++];
		switch (formatStr.buffer[++i]) {
		case 'b': 
			String_AppendInt(str, *((uint8_t*)arg)); break;
		case 'i': 
			String_AppendInt(str, *((int*)arg)); break;
		case 'f': 
			digits = formatStr.buffer[++i] - '0';
			String_AppendFloat(str, *((float*)arg), digits); break;
		case 'p':
			digits = formatStr.buffer[++i] - '0';
			String_AppendPaddedInt(str, *((int*)arg), digits); break;
		case 't': 
			String_AppendBool(str, *((bool*)arg)); break;
		case 'c': 
			String_AppendConst(str, (char*)arg);  break;
		case 's': 
			String_AppendString(str, (String*)arg);  break;
		case 'r':
			String_Append(str, *((char*)arg)); break;
		case 'x':
			if (sizeof(uintptr_t) == 4) {
				String_Hex32(str, *((uint32_t*)arg)); break;
			} else {
				String_Hex64(str, *((uint64_t*)arg)); break;
			}
		case 'h':
			String_Hex32(str, *((uint32_t*)arg)); break;
		default: 
			ErrorHandler_Fail("Invalid type for string format");
		}
	}
}


/*########################################################################################################################*
*-------------------------------------------------------Conversions-------------------------------------------------------*
*#########################################################################################################################*/
Codepoint Convert_ControlChars[32] = {
	0x0000, 0x263A, 0x263B, 0x2665, 0x2666, 0x2663, 0x2660, 0x2022,
	0x25D8, 0x25CB, 0x25D9, 0x2642, 0x2640, 0x266A, 0x266B, 0x263C,
	0x25BA, 0x25C4, 0x2195, 0x203C, 0x00B6, 0x00A7, 0x25AC, 0x21A8,
	0x2191, 0x2193, 0x2192, 0x2190, 0x221F, 0x2194, 0x25B2, 0x25BC
};

Codepoint Convert_ExtendedChars[129] = { 0x2302,
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

Codepoint Convert_CP437ToUnicode(char c) {
	uint8_t raw = (uint8_t)c;
	if (raw < 0x20) return Convert_ControlChars[raw];
	if (raw < 0x7F) return raw;
	return Convert_ExtendedChars[raw - 0x7F];
}

char Convert_UnicodeToCP437(Codepoint cp) {
	char value; Convert_TryUnicodeToCP437(cp, &value); return value;
}

bool Convert_TryUnicodeToCP437(Codepoint cp, char* value) {
	if (cp >= 0x20 && cp < 0x7F) { *value = (char)cp; return true; }
	int i;

	for (i = 0; i < Array_Elems(Convert_ControlChars); i++) {
		if (Convert_ControlChars[i] == cp) { *value = i; return true; }
	}
	for (i = 0; i < Array_Elems(Convert_ExtendedChars); i++) {
		if (Convert_ExtendedChars[i] == cp) { *value = i + 0x7F; return true; }
	}

	*value = '?'; return false;
}

void String_DecodeUtf8(String* str, uint8_t* data, uint32_t len) {
	struct Stream mem; Stream_ReadonlyMemory(&mem, data, len);
	Codepoint cp;

	while (mem.Meta.Mem.Left) {
		ReturnCode res = Stream_ReadUtf8(&mem, &cp);
		if (res) break; /* Memory read only returns ERR_END_OF_STREAM */
		String_Append(str, Convert_UnicodeToCP437(cp));
	}
}

bool Convert_TryParseUInt8(const String* str, uint8_t* value) {
	*value = 0; int tmp;
	if (!Convert_TryParseInt(str, &tmp) || tmp < 0 || tmp > UInt8_MaxValue) return false;
	*value = (uint8_t)tmp; return true;
}

bool Convert_TryParseInt16(const String* str, int16_t* value) {
	*value = 0; int tmp;
	if (!Convert_TryParseInt(str, &tmp) || tmp < Int16_MinValue || tmp > Int16_MaxValue) return false;
	*value = (int16_t)tmp; return true;
}

bool Convert_TryParseUInt16(const String* str, uint16_t* value) {
	*value = 0; int tmp;
	if (!Convert_TryParseInt(str, &tmp) || tmp < 0 || tmp > UInt16_MaxValue) return false;
	*value = (uint16_t)tmp; return true;
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

static bool Convert_TryParseDigits(const String* str, bool* negative, char* digits, int maxDigits) {
	*negative = false;
	if (!str->length) return false;
	char* start = digits; digits += (maxDigits - 1);

	/* Handle number signs */
	int offset = 0, i = 0;
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

bool Convert_TryParseInt(const String* str, int* value) {
	*value = 0;
	#define INT32_DIGITS 10
	bool negative;	
	char digits[INT32_DIGITS];
	if (!Convert_TryParseDigits(str, &negative, digits, INT32_DIGITS)) return false;

	int i, compare;
	if (negative) {
		compare = Convert_CompareDigits(digits, "2147483648");
		/* Special case, since |largest min value| is > |largest max value| */
		if (compare == 0) { *value = Int32_MinValue; return true; }
	} else {
		compare = Convert_CompareDigits(digits, "2147483647");
	}

	if (compare > 0) return false;	
	int sum = 0;
	for (i = 0; i < INT32_DIGITS; i++) {
		sum *= 10; sum += digits[i] - '0';
	}

	if (negative) sum = -sum;
	*value = sum;
	return true;
}

bool Convert_TryParseUInt64(const String* str, uint64_t* value) {
	*value = 0;
	#define UINT64_DIGITS 20
	bool negative;
	char digits[UINT64_DIGITS];
	if (!Convert_TryParseDigits(str, &negative, digits, UINT64_DIGITS)) return false;

	int i, compare = Convert_CompareDigits(digits, "18446744073709551615");
	if (negative || compare > 0) return false;

	uint64_t sum = 0;
	for (i = 0; i < UINT64_DIGITS; i++) {
		sum *= 10; sum += digits[i] - '0';
	}

	*value = sum;
	return true;
}

bool Convert_TryParseFloat(const String* str, float* value) {
	int i = 0;
	*value = 0.0f;
	bool foundDecimalPoint = false;
	double whole = 0.0, fract = 0.0, divide = 10.0;

	/* Handle number signs */
	bool negate = false;
	if (!str->length) return false;
	if (str->buffer[0] == '-') { negate = true; i++; }
	if (str->buffer[0] == '+') { i++; }

	/* TODO: CHECK THIS WORKS!!! */
	for (; i < str->length; i++) {
		char c = str->buffer[i];
		/* Floating point number can have . in it */
		if (c == '.' || c == ',') {
			if (foundDecimalPoint) return false;
			foundDecimalPoint = true;
			continue;
		}

		if (c < '0' || c > '9') return false;
		int digit = c - '0';
		if (!foundDecimalPoint) {
			whole *= 10; whole += digit;
		} else {
			fract += digit / divide; divide *= 10;
		}
	}

	double sum = whole + fract;
	if (negate) sum = -sum;
	*value = (float)sum;
	return true;
}

bool Convert_TryParseBool(const String* str, bool* value) {
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
#define STRINGSBUFFER_LEN_SHIFT 9
#define STRINGSBUFFER_LEN_MASK  0x1FFUL
#define STRINGSBUFFER_BUFFER_EXPAND_SIZE 8192

NOINLINE_ static void StringsBuffer_Init(StringsBuffer* buffer) {
	buffer->Count     = 0;
	buffer->TotalLength = 0;
	buffer->TextBuffer  = buffer->_DefaultBuffer;
	buffer->FlagsBuffer = buffer->_DefaultFlags;
	buffer->_TextBufferSize  = STRINGSBUFFER_BUFFER_DEF_SIZE;
	buffer->_FlagsBufferSize = STRINGSBUFFER_FLAGS_DEF_ELEMS;
}

void StringsBuffer_Clear(StringsBuffer* buffer) {
	/* Never initialised to begin with */
	if (!buffer->_FlagsBufferSize) return;

	if (buffer->TextBuffer != buffer->_DefaultBuffer) {
		Mem_Free(buffer->TextBuffer);
	}
	if (buffer->FlagsBuffer != buffer->_DefaultFlags) {
		Mem_Free(buffer->FlagsBuffer);
	}
	StringsBuffer_Init(buffer);
}

void StringsBuffer_Get(StringsBuffer* buffer, int i, String* text) {
	String raw = StringsBuffer_UNSAFE_Get(buffer, i);
	String_Copy(text, &raw);
}

String StringsBuffer_UNSAFE_Get(StringsBuffer* buffer, int i) {
	if (i < 0 || i >= buffer->Count) ErrorHandler_Fail("Tried to get String past StringsBuffer end");

	uint32_t flags  = buffer->FlagsBuffer[i];
	uint32_t offset = flags >> STRINGSBUFFER_LEN_SHIFT;
	uint32_t len    = flags  & STRINGSBUFFER_LEN_MASK;
	return String_Init(&buffer->TextBuffer[offset], len, len);
}

void StringsBuffer_Add(StringsBuffer* buffer, const String* text) {
	/* StringsBuffer hasn't been initalised yet, do it here */
	if (!buffer->_FlagsBufferSize) { StringsBuffer_Init(buffer); }

	if (buffer->Count == buffer->_FlagsBufferSize) {
		buffer->FlagsBuffer = Utils_Resize(buffer->FlagsBuffer, &buffer->_FlagsBufferSize, 
											4, STRINGSBUFFER_FLAGS_DEF_ELEMS, 512);
	}

	if (text->length > STRINGSBUFFER_LEN_MASK) {
		ErrorHandler_Fail("String too big to insert into StringsBuffer");
	}

	int textOffset = buffer->TotalLength;
	if (textOffset + text->length >= buffer->_TextBufferSize) {
		buffer->TextBuffer = Utils_Resize(buffer->TextBuffer, &buffer->_TextBufferSize,
											1, STRINGSBUFFER_BUFFER_DEF_SIZE, 8192);
	}

	if (text->length) {
		Mem_Copy(&buffer->TextBuffer[textOffset], text->buffer, text->length);
	}
	buffer->FlagsBuffer[buffer->Count] = text->length | (textOffset << STRINGSBUFFER_LEN_SHIFT);

	buffer->Count++;
	buffer->TotalLength += text->length;
}

void StringsBuffer_Remove(StringsBuffer* buffer, int index) {
	if (index < 0 || index >= buffer->Count) ErrorHandler_Fail("Tried to remove String past StringsBuffer end");

	uint32_t flags  = buffer->FlagsBuffer[index];
	uint32_t offset = flags >> STRINGSBUFFER_LEN_SHIFT;
	uint32_t len    = flags  & STRINGSBUFFER_LEN_MASK;

	/* Imagine buffer is this: AAXXYYZZ, and want to delete X */
	/* Start points to first character of Y */
	/* End points to character past last character of Z */
	uint32_t i, start = offset + len, end = buffer->TotalLength;
	for (i = start; i < end; i++) { 
		buffer->TextBuffer[i - len] = buffer->TextBuffer[i]; 
	}

	/* adjust text offset of elements after this element */
	/* Elements may not be in order so most account for that */
	uint32_t flagsLen = len << STRINGSBUFFER_LEN_SHIFT;
	for (i = index; i < buffer->Count; i++) {
		buffer->FlagsBuffer[i] = buffer->FlagsBuffer[i + 1];
		if (buffer->FlagsBuffer[i] >= flags) {
			buffer->FlagsBuffer[i] -= flagsLen;
		}
	}

	buffer->Count--;
	buffer->TotalLength -= len;
}


/*########################################################################################################################*
*------------------------------------------------------Word wrapper-------------------------------------------------------*
*#########################################################################################################################*/
bool WordWrap_IsWrapper(char c) {
	return c == '\0' || c == ' ' || c == '-' || c == '>' || c == '<' || c == '/' || c == '\\';
}

void WordWrap_Do(STRING_REF String* text, String* lines, int numLines, int lineLen) {
	int i;
	for (i = 0; i < numLines; i++) { lines[i] = String_MakeNull(); }

	int lineStart = 0, lineEnd;
	for (i = 0; i < numLines; i++) {
		int nextLineStart = lineStart + lineLen;
		/* No more text to wrap */
		if (nextLineStart >= text->length) {
			lines[i] = String_UNSAFE_SubstringAt(text, lineStart); return;
		}

		/* Find beginning of last word on current line */
		for (lineEnd = nextLineStart; lineEnd >= lineStart; lineEnd--) {
			if (WordWrap_IsWrapper(text->buffer[lineEnd])) break;
		}
		lineEnd++; /* move after wrapper char (i.e. beginning of last word)*/

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

void WordWrap_GetCoords(int index, const String* lines, int numLines, int* coordX, int* coordY) {
	if (index == -1) index = Int32_MaxValue;
	int offset = 0; *coordX = -1; *coordY = 0;

	int y;
	for (y = 0; y < numLines; y++) {
		int lineLength = lines[y].length;
		if (!lineLength) break;

		*coordY = y;
		if (index < offset + lineLength) {
			*coordX = index - offset; break;
		}
		offset += lineLength;
	}
	if (*coordX == -1) *coordX = lines[*coordY].length;
}

int WordWrap_GetBackLength(const String* text, int index) {
	if (index <= 0) return 0;
	if (index >= text->length) {
		ErrorHandler_Fail("WordWrap_GetBackLength - index past end of string");
	}

	int start = index;
	bool lookingSpace = text->buffer[index] == ' ';
	/* go back to the end of the previous word */
	if (lookingSpace) {
		while (index > 0 && text->buffer[index] == ' ') { index--; }
	}

	/* go back to the start of the current word */
	while (index > 0 && text->buffer[index] != ' ') { index--; }
	return start - index;
}

int WordWrap_GetForwardLength(const String* text, int index) {
	if (index == -1) return 0;
	if (index >= text->length) {
		ErrorHandler_Fail("WordWrap_GetForwardLength - index past end of string");
	}

	int start = index, length = text->length;
	bool lookingLetter = text->buffer[index] != ' ';
	/* go forward to the end of the current word */
	if (lookingLetter) {
		while (index < length && text->buffer[index] != ' ') { index++; }
	}

	/* go forward to the start of the next word */
	while (index < length && text->buffer[index] == ' ') { index++; }
	return index - start;
}
