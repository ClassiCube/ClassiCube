#include "String.h"
#include "Funcs.h"
#include "ErrorHandler.h"
#include "Platform.h"
#include "Stream.h"
#include "Utils.h"

const String String_Empty;

String String_Init(STRING_REF char* buffer, int length, int capacity) {
	String s; 
	s.buffer = buffer; s.length = length; s.capacity = capacity; 
	return s;
}

String String_InitAndClear(STRING_REF char* buffer, int capacity) {
	String str = String_Init(buffer, 0, capacity);	
	int i;
	for (i = 0; i < capacity; i++) { buffer[i] = '\0'; }
	return str;
}

int String_CalcLen(const char* raw, int capacity) {
	int length = 0;
	while (length < capacity && *raw) { raw++; length++; }
	return length;
}

String String_FromRaw(STRING_REF char* buffer, int capacity) {
	return String_Init(buffer, String_CalcLen(buffer, capacity), capacity);
}

String String_FromReadonly(STRING_REF const char* buffer) {
	int len = String_CalcLen(buffer, UInt16_MaxValue);
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

int String_UNSAFE_Split(STRING_REF const String* str, char c, String* subs, int maxSubs) {
	int start = 0, end, count, i;

	for (i = 0; i < maxSubs && start <= str->length; i++) {
		end = String_IndexOf(str, c, start);
		if (end == -1) end = str->length;

		subs[i] = String_UNSAFE_Substring(str, start, end - start);
		start = end + 1;
	}

	count = i;
	/* If not enough split substrings, make remaining NULL */
	for (; i < maxSubs; i++) { subs[i] = String_Empty; }
	return count;
}

bool String_UNSAFE_Separate(STRING_REF const String* str, char c, String* key, String* value) {
	int idx = String_IndexOf(str, c, 0);
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


bool String_Equals(const String* a, const String* b) {
	int i;
	if (a->length != b->length) return false;

	for (i = 0; i < a->length; i++) {
		if (a->buffer[i] != b->buffer[i]) return false;
	}
	return true;
}

bool String_CaselessEquals(const String* a, const String* b) {
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

bool String_CaselessEqualsConst(const String* a, const char* b) {
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


void String_Append(String* str, char c) {
	if (str->length == str->capacity) return;

	str->buffer[str->length] = c;
	str->length++;
}

void String_AppendBool(String* str, bool value) {
	const char* text = value ? "True" : "False";
	String_AppendConst(str, text);
}

int String_MakeUInt32(uint32_t num, char* digits) {
	int len = 0;
	do {
		digits[len] = '0' + (num % 10); num /= 10; len++;
	} while (num > 0);
	return len;
}

void String_AppendInt(String* str, int num) {
	if (num < 0) {
		num = -num;
		String_Append(str, '-');
	}
	String_AppendUInt32(str, (uint32_t)num);
}

void String_AppendUInt32(String* str, uint32_t num) {
	char digits[STRING_INT_CHARS];
	int i, count = String_MakeUInt32(num, digits);

	for (i = count - 1; i >= 0; i--) {
		String_Append(str, digits[i]);
	}
}

void String_AppendPaddedInt(String* str, int num, int minDigits) {
	char digits[STRING_INT_CHARS];
	int i, count;
	for (i = 0; i < minDigits; i++) { digits[i] = '0'; }

	count = String_MakeUInt32(num, digits);
	if (count < minDigits) count = minDigits;

	for (i = count - 1; i >= 0; i--) {
		String_Append(str, digits[i]);
	}
}

int String_MakeUInt64(uint64_t num, char* digits) {
	int len = 0;
	do {
		digits[len] = '0' + (num % 10); num /= 10; len++;
	} while (num > 0);
	return len;
}

void String_AppendUInt64(String* str, uint64_t num) {
	char digits[STRING_INT_CHARS];
	int i, count = String_MakeUInt64(num, digits);

	for (i = count - 1; i >= 0; i--) {
		String_Append(str, digits[i]);
	}
}

void String_AppendFloat(String* str, float num, int fracDigits) {
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

void String_AppendHex(String* str, uint8_t value) {
	/* 48 = index of 0, 55 = index of (A - 10) */
	uint8_t hi = (value >> 4) & 0xF;
	char c_hi  = hi < 10 ? (hi + 48) : (hi + 55);
	uint8_t lo = value & 0xF;
	char c_lo  = lo < 10 ? (lo + 48) : (lo + 55);

	String_Append(str, c_hi);
	String_Append(str, c_lo);
}

CC_NOINLINE static void String_Hex32(String* str, uint32_t value) {
	int shift;

	for (shift = 24; shift >= 0; shift -= 8) {
		uint8_t part = (uint8_t)(value >> shift);
		String_AppendHex(str, part);
	}
}

CC_NOINLINE static void String_Hex64(String* str, uint64_t value) {
	int shift;

	for (shift = 56; shift >= 0; shift -= 8) {
		uint8_t part = (uint8_t)(value >> shift);
		String_AppendHex(str, part);
	}
}

void String_AppendConst(String* str, const char* src) {
	for (; *src; src++) {
		String_Append(str, *src);
	}
}

void String_AppendString(String* str, const String* src) {
	int i;
	for (i = 0; i < src->length; i++) {
		String_Append(str, src->buffer[i]);
	}
}

void String_AppendColorless(String* str, const String* src) {
	char c;
	int i;

	for (i = 0; i < src->length; i++) {
		c = src->buffer[i];
		if (c == '&') { i++; continue; } /* Skip over the following colour code */
		String_Append(str, c);
	}
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

void String_DeleteAt(String* str, int offset) {
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

void String_UNSAFE_TrimStart(String* str) {
	int i;
	for (i = 0; i < str->length; i++) {
		if (str->buffer[i] != ' ') break;
			
		str->buffer++;
		str->length--; i--;
	}
}

void String_UNSAFE_TrimEnd(String* str) {
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

bool String_CaselessStarts(const String* str, const String* sub) {
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

bool String_CaselessEnds(const String* str, const String* sub) {
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

int String_Compare(const String* a, const String* b) {
	char aCur, bCur;
	int i, minLen = min(a->length, b->length);

	for (i = 0; i < minLen; i++) {
		aCur = a->buffer[i]; Char_MakeLower(aCur);
		bCur = b->buffer[i]; Char_MakeLower(bCur);

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
	const void* arg;
	int i, j = 0, digits;

	const void* args[4];
	args[0] = a1; args[1] = a2; args[2] = a3; args[3] = a4;

	for (i = 0; i < formatStr.length; i++) {
		if (formatStr.buffer[i] != '%') { String_Append(str, formatStr.buffer[i]); continue; }
		arg = args[j++];

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
static Codepoint Convert_ControlChars[32] = {
	0x0000, 0x263A, 0x263B, 0x2665, 0x2666, 0x2663, 0x2660, 0x2022,
	0x25D8, 0x25CB, 0x25D9, 0x2642, 0x2640, 0x266A, 0x266B, 0x263C,
	0x25BA, 0x25C4, 0x2195, 0x203C, 0x00B6, 0x00A7, 0x25AC, 0x21A8,
	0x2191, 0x2193, 0x2192, 0x2190, 0x221F, 0x2194, 0x25B2, 0x25BC
};

static Codepoint Convert_ExtendedChars[129] = { 0x2302,
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
	char c; Convert_TryUnicodeToCP437(cp, &c); return c;
}

bool Convert_TryUnicodeToCP437(Codepoint cp, char* c) {
	int i;
	if (cp >= 0x20 && cp < 0x7F) { *c = (char)cp; return true; }

	for (i = 0; i < Array_Elems(Convert_ControlChars); i++) {
		if (Convert_ControlChars[i] == cp) { *c = i; return true; }
	}
	for (i = 0; i < Array_Elems(Convert_ExtendedChars); i++) {
		if (Convert_ExtendedChars[i] == cp) { *c = i + 0x7F; return true; }
	}

	*c = '?'; return false;
}

void String_DecodeUtf8(String* str, uint8_t* data, uint32_t len) {
	Codepoint cp;
	int read;

	while (len) {
		read = Convert_Utf8ToUnicode(&cp, data, len);
		if (!read) break;

		String_Append(str, Convert_UnicodeToCP437(cp));
		data += read; len -= read;
	}
}

int Convert_Utf8ToUnicode(Codepoint* cp, const uint8_t* data, uint32_t len) {
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
			| ((data[2] & 0x3F) << 6)  | (data[3] & 0x3F);
		return 4;
	}
}

int Convert_UnicodeToUtf8(Codepoint cp, uint8_t* data) {
	if (cp <= 0x7F) {
		data[0] = (uint8_t)cp;
		return 1;
	} else if (cp <= 0x7FF) {
		data[0] = 0xC0 | ((cp >> 6) & 0x1F);
		data[1] = 0x80 | ((cp)      & 0x3F);
		return 2;
	} else {
		data[0] = 0xE0 | ((cp >> 12) & 0x0F);
		data[1] = 0x80 | ((cp >>  6) & 0x3F);
		data[2] = 0x80 | ((cp)       & 0x3F);
		return 3;
	}
}


/*########################################################################################################################*
*--------------------------------------------------Numerical conversions--------------------------------------------------*
*#########################################################################################################################*/
bool Convert_ParseUInt8(const String* str, uint8_t* value) {
	int tmp; 
	*value = 0;
	if (!Convert_ParseInt(str, &tmp) || tmp < 0 || tmp > UInt8_MaxValue) return false;
	*value = (uint8_t)tmp; return true;
}

bool Convert_ParseInt16(const String* str, int16_t* value) {
	int tmp; 
	*value = 0;
	if (!Convert_ParseInt(str, &tmp) || tmp < Int16_MinValue || tmp > Int16_MaxValue) return false;
	*value = (int16_t)tmp; return true;
}

bool Convert_ParseUInt16(const String* str, uint16_t* value) {
	int tmp; 
	*value = 0;
	if (!Convert_ParseInt(str, &tmp) || tmp < 0 || tmp > UInt16_MaxValue) return false;
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
bool Convert_ParseInt(const String* str, int* value) {
	bool negative;
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
bool Convert_ParseUInt64(const String* str, uint64_t* value) {
	bool negative;
	char digits[UINT64_DIGITS];
	int i, compare;
	uint64_t sum = 0;

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

bool Convert_ParseFloat(const String* str, float* value) {
	int i = 0;
	bool negate = false;
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

bool Convert_ParseBool(const String* str, bool* value) {
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

CC_NOINLINE static void StringsBuffer_Init(StringsBuffer* buffer) {
	buffer->Count       = 0;
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

void StringsBuffer_Get(StringsBuffer* buffer, int i, String* str) {
	String entry = StringsBuffer_UNSAFE_Get(buffer, i);
	String_Copy(str, &entry);
}

String StringsBuffer_UNSAFE_Get(StringsBuffer* buffer, int i) {
	uint32_t flags, offset, len;
	if (i < 0 || i >= buffer->Count) Logger_Abort("Tried to get String past StringsBuffer end");

	flags  = buffer->FlagsBuffer[i];
	offset = flags >> STRINGSBUFFER_LEN_SHIFT;
	len    = flags  & STRINGSBUFFER_LEN_MASK;
	return String_Init(&buffer->TextBuffer[offset], len, len);
}

void StringsBuffer_Add(StringsBuffer* buffer, const String* str) {
	int textOffset;
	/* StringsBuffer hasn't been initalised yet, do it here */
	if (!buffer->_FlagsBufferSize) { StringsBuffer_Init(buffer); }

	if (buffer->Count == buffer->_FlagsBufferSize) {
		buffer->FlagsBuffer = Utils_Resize(buffer->FlagsBuffer, &buffer->_FlagsBufferSize, 
											4, STRINGSBUFFER_FLAGS_DEF_ELEMS, 512);
	}

	if (str->length > STRINGSBUFFER_LEN_MASK) {
		Logger_Abort("String too big to insert into StringsBuffer");
	}

	textOffset = buffer->TotalLength;
	if (textOffset + str->length >= buffer->_TextBufferSize) {
		buffer->TextBuffer = Utils_Resize(buffer->TextBuffer, &buffer->_TextBufferSize,
											1, STRINGSBUFFER_BUFFER_DEF_SIZE, 8192);
	}

	Mem_Copy(&buffer->TextBuffer[textOffset], str->buffer, str->length);
	buffer->FlagsBuffer[buffer->Count] = str->length | (textOffset << STRINGSBUFFER_LEN_SHIFT);

	buffer->Count++;
	buffer->TotalLength += str->length;
}

void StringsBuffer_Remove(StringsBuffer* buffer, int index) {
	uint32_t flags, offset, len;
	uint32_t i, offsetAdj;
	if (index < 0 || index >= buffer->Count) Logger_Abort("Tried to remove String past StringsBuffer end");

	flags  = buffer->FlagsBuffer[index];
	offset = flags >> STRINGSBUFFER_LEN_SHIFT;
	len    = flags  & STRINGSBUFFER_LEN_MASK;

	/* Imagine buffer is this: AAXXYYZZ, and want to delete XX */
	/* We iterate from first char of Y to last char of Z, */
	/* shifting each character two to the left. */
	for (i = offset + len; i < buffer->TotalLength; i++) {
		buffer->TextBuffer[i - len] = buffer->TextBuffer[i]; 
	}

	/* Adjust text offset of elements after this element */
	/* Elements may not be in order so must account for that */
	offsetAdj = len << STRINGSBUFFER_LEN_SHIFT;
	for (i = index; i < buffer->Count - 1; i++) {
		buffer->FlagsBuffer[i] = buffer->FlagsBuffer[i + 1];
		if (buffer->FlagsBuffer[i] >= flags) {
			buffer->FlagsBuffer[i] -= offsetAdj;
		}
	}
	
	buffer->Count--;
	buffer->TotalLength -= len;
}


/*########################################################################################################################*
*------------------------------------------------------Word wrapper-------------------------------------------------------*
*#########################################################################################################################*/
static bool WordWrap_IsWrapper(char c) {
	return c == '\0' || c == ' ' || c == '-' || c == '>' || c == '<' || c == '/' || c == '\\';
}

void WordWrap_Do(STRING_REF String* text, String* lines, int numLines, int lineLen) {
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

int WordWrap_GetBackLength(const String* text, int index) {
	int start = index;
	if (index <= 0) return 0;
	if (index >= text->length) Logger_Abort("WordWrap_GetBackLength - index past end of string");
	
	/* Go backward to the end of the previous word */
	while (index > 0 && text->buffer[index] == ' ') index--;
	/* Go backward to the start of the current word */
	while (index > 0 && text->buffer[index] != ' ') index--;

	return start - index;
}

int WordWrap_GetForwardLength(const String* text, int index) {
	int start = index, length = text->length;
	if (index == -1) return 0;
	if (index >= text->length) Logger_Abort("WordWrap_GetForwardLength - index past end of string");

	/* Go forward to the end of the word 'index' is currently in */
	while (index < length && text->buffer[index] != ' ') index++;
	/* Go forward to the start of the next word after 'index' */
	while (index < length && text->buffer[index] == ' ') index++;

	return index - start;
}
