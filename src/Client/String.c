#include "String.h"
#include "Funcs.h"
#include "ErrorHandler.h"
#include "Platform.h"
#include "ExtMath.h"

#define Char_MakeLower(ch) if ((ch) >= 'A' && (ch) <= 'Z') { (ch) += ' '; }
UInt8 Char_ToLower(UInt8 c) {
	Char_MakeLower(c);
	return c;
}

String String_Init(STRING_REF UInt8* buffer, UInt16 length, UInt16 capacity) {
	String str;
	str.buffer = buffer;
	str.length = length;
	str.capacity = capacity;
	return str;
}

String String_InitAndClear(STRING_REF UInt8* buffer, UInt16 capacity) {
	String str = String_Init(buffer, 0, capacity);	
	Int32 i;

	/* Need to set region occupied by string to NULL for interop with native APIs */
	for (i = 0; i < capacity + 1; i++) { buffer[i] = NULL; }
	return str;
}

String String_MakeNull(void) { return String_Init(NULL, 0, 0); }

String String_FromRaw(STRING_REF UInt8* buffer, UInt16 capacity) {
	UInt16 length = 0;
	UInt8* cur = buffer;
	while (length < capacity && *cur != NULL) { cur++; length++; }

	return String_Init(buffer, length, capacity);
}

String String_FromReadonly(STRING_REF const UInt8* buffer) {
	String str = String_FromRaw(buffer, UInt16_MaxValue);
	str.capacity = str.length;
	return str;
}


void String_MakeLowercase(STRING_TRANSIENT String* str) {
	Int32 i;
	for (i = 0; i < str->length; i++) {
		str->buffer[i] = Char_ToLower(str->buffer[i]);
	}
}

void String_StripCols(STRING_TRANSIENT String* str) {
	Int32 i;
	for (i = str->length - 1; i >= 0; i--) {
		if (str->buffer[i] != '&') continue;
		/* Remove the & and the colour code following it */
		String_DeleteAt(str, i); String_DeleteAt(str, i);
	}
}

void String_Clear(STRING_TRANSIENT String* str) {
	Int32 i;
	for (i = 0; i < str->length; i++) { 
		str->buffer[i] = NULL; 
	}
	str->length = 0;
}

void String_Set(STRING_TRANSIENT String* str, STRING_PURE String* value) {
	String_Clear(str);
	String_AppendString(str, value);
}

String String_UNSAFE_Substring(STRING_REF String* str, Int32 offset, Int32 length) {
	if (offset < 0 || offset > str->length) {
		ErrorHandler_Fail("Offset for substring out of range");
	}
	if (length < 0 || length > str->length) {
		ErrorHandler_Fail("Length for substring out of range");
	}
	if (offset + length > str->length) {
		ErrorHandler_Fail("Result substring is out of range");
	}
	return String_Init(str->buffer + offset, (UInt16)length, (UInt16)length);
}

void String_UNSAFE_Split(STRING_REF String* str, UInt8 c, STRING_TRANSIENT String* subs, UInt32* subsCount) {
	UInt32 maxSubs = *subsCount, i = 0;
	Int32 start = 0;
	for (; i < maxSubs && start <= str->length; i++) {
		Int32 end = String_IndexOf(str, c, start);
		if (end == -1) end = str->length;

		subs[i] = String_UNSAFE_Substring(str, start, end - start);
		start = end + 1;
	}

	*subsCount = i;
	/* If not enough split substrings, make remaining null */
	for (; i < maxSubs; i++) { subs[i] = String_MakeNull(); }
}


bool String_Equals(STRING_PURE String* a, STRING_PURE String* b) {
	if (a->length != b->length) return false;
	Int32 i;

	for (i = 0; i < a->length; i++) {
		if (a->buffer[i] != b->buffer[i]) return false;
	}
	return true;
}

bool String_CaselessEquals(STRING_PURE String* a, STRING_PURE String* b) {
	if (a->length != b->length) return false;
	Int32 i;

	for (i = 0; i < a->length; i++) {
		UInt8 aCur = a->buffer[i]; Char_MakeLower(aCur);
		UInt8 bCur = b->buffer[i]; Char_MakeLower(bCur);
		if (aCur != bCur) return false;
	}
	return true;
}

bool String_CaselessEqualsConst(STRING_PURE String* a, STRING_PURE const UInt8* b) {
	Int32 i;

	for (i = 0; i < a->length; i++) {
		UInt8 aCur = a->buffer[i]; Char_MakeLower(aCur);
		UInt8 bCur = b[i];         Char_MakeLower(bCur);
		if (aCur != bCur || bCur == NULL) return false;
	}
	/* ensure at end of string */
	return b[a->length] == NULL;
}


bool String_Append(STRING_TRANSIENT String* str, UInt8 c) {
	if (str->length == str->capacity) return false;

	str->buffer[str->length] = c;
	str->length++;
	return true;
}

bool String_AppendBool(STRING_TRANSIENT String* str, bool value) {
	UInt8* text = value ? "True" : "False";
	return String_AppendConst(str, text);
}

Int32 String_MakeUInt32(UInt32 num, UInt8* numBuffer) {
	Int32 len = 0;
	do {
		numBuffer[len] = '0' + (num % 10); num /= 10; len++;
	} while (num > 0);
	return len;
}

bool String_AppendInt32(STRING_TRANSIENT String* str, Int32 num) {
	if (num < 0) {
		num = -num;
		if (!String_Append(str, '-')) return false;
	}
	return String_AppendUInt32(str, (UInt32)num);
}

bool String_AppendUInt32(STRING_TRANSIENT String* str, UInt32 num) {
	UInt8 numBuffer[STRING_INT_CHARS];
	Int32 i, numLen = String_MakeUInt32(num, numBuffer);

	for (i = numLen - 1; i >= 0; i--) {
		if (!String_Append(str, numBuffer[i])) return false;
	}
	return true;
}

/* Attempts to append an integer value to the end of a string, padding left with 0. */
bool String_AppendPaddedInt32(STRING_TRANSIENT String* str, Int32 num, Int32 minDigits) {
	UInt8 numBuffer[STRING_INT_CHARS];
	Int32 i;
	for (i = 0; i < minDigits; i++) { numBuffer[i] = '0'; }

	Int32 numLen = String_MakeUInt32(num, numBuffer);
	if (numLen < minDigits) numLen = minDigits;

	for (i = numLen - 1; i >= 0; i--) {
		if (!String_Append(str, numBuffer[i])) return false;
	}
	return true;
}

Int32 String_MakeUInt64(UInt64 num, UInt8* numBuffer) {
	Int32 len = 0;
	do {
		numBuffer[len] = '0' + (num % 10); num /= 10; len++;
	} while (num > 0);
	return len;
}

bool String_AppendInt64(STRING_TRANSIENT String* str, Int64 num) {
	if (num < 0) {
		num = -num;
		if (!String_Append(str, '-')) return false;
	}

	UInt8 numBuffer[STRING_INT_CHARS];
	Int32 i, numLen = String_MakeUInt64(num, numBuffer);

	for (i = numLen - 1; i >= 0; i--) {
		if (!String_Append(str, numBuffer[i])) return false;
	}
	return true;
}

bool String_AppendReal32(STRING_TRANSIENT String* str, Real32 num, Int32 fracDigits) {
	Int32 wholePortion = (Int32)num;
	if (!String_AppendInt32(str, wholePortion)) return false;

	Real64 frac = (Real64)num - (Real64)wholePortion;
	if (frac == 0.0) return true;
	if (!String_Append(str, '.')) return false;

	Int32 i;
	/* TODO: negative numbers here */
	for (i = 0; i < fracDigits; i++) {
		frac *= 10;
		Int32 digit = Math_AbsI((Int32)frac) % 10;
		if (!String_Append(str, '0' + digit)) return false;
	}
	return true;
}

bool String_AppendConst(STRING_TRANSIENT String* str, const UInt8* toAppend) {
	UInt8 cur = 0;

	while ((cur = *toAppend) != 0) {
		if (!String_Append(str, cur)) return false;
		toAppend++;
	}
	return true;
}

bool String_AppendString(STRING_TRANSIENT String* str, STRING_PURE String* toAppend) {
	Int32 i;

	for (i = 0; i < toAppend->length; i++) {
		if (!String_Append(str, toAppend->buffer[i])) return false;
	}
	return true;
}

bool String_AppendColorless(STRING_TRANSIENT String* str, STRING_PURE String* toAppend) {
	Int32 i;

	for (i = 0; i < toAppend->length; i++) {
		UInt8 c = toAppend->buffer[i];
		if (c == '&') { i++; continue; } /* Skip over the following colour code */
		if (!String_Append(str, c)) return false;
	}
	return true;
}


Int32 String_IndexOf(STRING_PURE String* str, UInt8 c, Int32 offset) {
	Int32 i;
	for (i = offset; i < str->length; i++) {
		if (str->buffer[i] == c) return i;
	}
	return -1;
}

Int32 String_LastIndexOf(STRING_PURE String* str, UInt8 c) {
	Int32 i;
	for (i = str->length - 1; i >= 0; i--) {
		if (str->buffer[i] == c) return i;
	}
	return -1;
}

void String_InsertAt(STRING_TRANSIENT String* str, Int32 offset, UInt8 c) {
	if (offset < 0 || offset > str->length) {
		ErrorHandler_Fail("Offset for InsertAt out of range");
	}
	if (str->length == str->capacity) {
		ErrorHandler_Fail("Cannot insert character into full string");
	}

	Int32 i;
	for (i = str->length; i > offset; i--) {
		str->buffer[i] = str->buffer[i - 1];
	}

	str->buffer[offset] = c;
	str->length++;
}

void String_DeleteAt(STRING_TRANSIENT String* str, Int32 offset) {
	if (offset < 0 || offset >= str->length) {
		ErrorHandler_Fail("Offset for DeleteAt out of range");
	}

	Int32 i;
	for (i = offset; i < str->length - 1; i++) {
		str->buffer[i] = str->buffer[i + 1];
	}

	str->buffer[str->length - 1] = NULL;
	str->length--;
}

void String_UNSAFE_TrimStart(STRING_TRANSIENT String* str) {
	Int32 i;
	for (i = 0; i < str->length; i++) {
		if (str->buffer[i] != ' ') break;
			
		str->buffer++;
		str->length--; i--;
	}
}

void String_UNSAFE_TrimEnd(STRING_TRANSIENT String* str) {
	Int32 i;
	for (i = str->length - 1; i >= 0; i--) {
		if (str->buffer[i] != ' ') break;
		str->length--;
	}
}

Int32 String_IndexOfString(STRING_PURE String* str, STRING_PURE String* sub) {
	Int32 i, j;
	/* Special case, sub is an empty string*/
	if (sub->length == 0) return 0;

	UInt8 subFirst = sub->buffer[0];
	for (i = 0; i < str->length; i++) {
		if (str->buffer[i] != subFirst) continue;
		
		for (j = 1; j < sub->length; j++) {
			if (str->buffer[i + j] != sub->buffer[j]) break;
		}
		if (j == sub->length) return i;
	}
	return -1;
}

bool String_StartsWith(STRING_PURE String* str, STRING_PURE String* sub) {
	if (str->length < sub->length) return false;
	Int32 i;

	for (i = 0; i < sub->length; i++) {
		if (str->buffer[i] != sub->buffer[i]) return false;
	}
	return true;
}

bool String_CaselessStarts(STRING_PURE String* str, STRING_PURE String* sub) {
	if (str->length < sub->length) return false;
	Int32 i;

	for (i = 0; i < sub->length; i++) {
		UInt8 strCur = str->buffer[i]; Char_MakeLower(strCur);
		UInt8 subCur = sub->buffer[i]; Char_MakeLower(subCur);
		if (strCur != subCur) return false;
	}
	return true;
}

bool String_CaselessEnds(STRING_PURE String* str, STRING_PURE String* sub) {
	if (str->length < sub->length) return false;
	Int32 i, j = str->length - sub->length;

	for (i = 0; i < sub->length; i++) {
		UInt8 strCur = str->buffer[j + i]; Char_MakeLower(strCur);
		UInt8 subCur = sub->buffer[i];     Char_MakeLower(subCur);
		if (strCur != subCur) return false;
	}
	return true;
}

Int32 String_Compare(STRING_PURE String* a, STRING_PURE String* b) {
	Int32 minLen = min(a->length, b->length);
	Int32 i;

	for (i = 0; i < minLen; i++) {
		if (a->buffer[i] == b->buffer[i]) continue;
		return a->buffer[i] < b->buffer[i] ? 1 : -1;
	}

	/* all chars are equal here - same string, or a substring */
	if (a->length == b->length) return 0;
	return a->length < b->length ? 1 : -1;
}

void String_Format1(STRING_TRANSIENT String* str, const UInt8* format, const void* a1) {
	String_Format4(str, format, a1, NULL, NULL, NULL);
}
void String_Format2(STRING_TRANSIENT String* str, const UInt8* format, const void* a1, const void* a2) {
	String_Format4(str, format, a1, a2, NULL, NULL);
}
void String_Format3(STRING_TRANSIENT String* str, const UInt8* format, const void* a1, const void* a2, const void* a3) {
	String_Format4(str, format, a1, a2, a3, NULL);
}
void String_Format4(STRING_TRANSIENT String* str, const UInt8* format, const void* a1, const void* a2, const void* a3, const void* a4) {
	String formatStr = String_FromReadonly(format);
	const void* args[4] = { a1, a2, a3, a4 };
	Int32 i, j = 0, digits;

	for (i = 0; i < formatStr.length; i++) {
		if (formatStr.buffer[i] != '%') { String_Append(str, formatStr.buffer[i]); continue; }

		const void* arg = args[j++];
		switch (formatStr.buffer[++i]) {
		case 'b': 
			String_AppendInt32(str, *((UInt8*)arg)); break;
		case 'i': 
			String_AppendInt32(str, *((Int32*)arg)); break;
		case 'f': 
			digits = formatStr.buffer[++i] - '0';
			String_AppendReal32(str, *((Real32*)arg), digits); break;
		case 'p':
			digits = formatStr.buffer[++i] - '0';
			String_AppendPaddedInt32(str, *((Int32*)arg), digits); break;
		case 't': 
			String_AppendBool(str, *((bool*)arg)); break;
		case 'c': 
			String_AppendConst(str, (UInt8*)arg);  break;
		case 's': 
			String_AppendString(str, (String*)arg);  break;
		case 'r':
			String_Append(str, *((UInt8*)arg)); break;
		default: 
			ErrorHandler_Fail("Invalid type for string format");
		}
	}
}


UInt16 Convert_ControlChars[32] = {
	0x0000, 0x263A, 0x263B, 0x2665, 0x2666, 0x2663, 0x2660, 0x2022,
	0x25D8, 0x25CB, 0x25D9, 0x2642, 0x2640, 0x266A, 0x266B, 0x263C,
	0x25BA, 0x25C4, 0x2195, 0x203C, 0x00B6, 0x00A7, 0x25AC, 0x21A8,
	0x2191, 0x2193, 0x2192, 0x2190, 0x221F, 0x2194, 0x25B2, 0x25BC
};

UInt16 Convert_ExtendedChars[129] = { 0x2302,
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

UInt16 Convert_CP437ToUnicode(UInt8 c) {
	if (c < 0x20) return Convert_ControlChars[c];
	if (c < 0x7F) return c;
	return Convert_ExtendedChars[c - 0x7F];
}

UInt8 Convert_UnicodeToCP437(UInt16 c) {
	UInt8 value; Convert_TryUnicodeToCP437(c, &value); return value;
}

bool Convert_TryUnicodeToCP437(UInt16 c, UInt8* value) {
	if (c >= 0x20 && c < 0x7F) { *value = (UInt8)c; return true; }
	UInt32 i;

	for (i = 0; i < Array_Elems(Convert_ControlChars); i++) {
		if (Convert_ControlChars[i] == c) { *value = (UInt8)i; return true; }
	}
	for (i = 0; i < Array_Elems(Convert_ExtendedChars); i++) {
		if (Convert_ExtendedChars[i] == c) { *value = (UInt8)(i + 0x7F); return true; }
	}

	*value = '?'; return false;
}

bool Convert_TryParseUInt8(STRING_PURE String* str, UInt8* value) {
	*value = 0; Int32 tmp;
	if (!Convert_TryParseInt32(str, &tmp) || tmp < 0 || tmp > UInt8_MaxValue) return false;
	*value = (UInt8)tmp; return true;
}

bool Convert_TryParseInt16(STRING_PURE String* str, Int16* value) {
	*value = 0; Int32 tmp;
	if (!Convert_TryParseInt32(str, &tmp) || tmp < Int16_MinValue || tmp > Int16_MaxValue) return false;
	*value = (Int16)tmp; return true;
}

bool Convert_TryParseUInt16(STRING_PURE String* str, UInt16* value) {
	*value = 0; Int32 tmp;
	if (!Convert_TryParseInt32(str, &tmp) || tmp < 0 || tmp > UInt16_MaxValue) return false;
	*value = (UInt16)tmp; return true;
}

Int32 Convert_CompareDigits(const UInt8* digits, const UInt8* magnitude) {
	Int32 i;
	for (i = 0; ; i++) {
		if (magnitude[i] == NULL)     return  0;
		if (digits[i] > magnitude[i]) return  1;
		if (digits[i] < magnitude[i]) return -1;
	}
	return 0;
}

bool Convert_TryParseDigits(STRING_PURE String* str, bool* negative, UInt8* digits, Int32 maxDigits) {
	*negative = false;
	if (str->length == 0) return false;
	UInt8* start = digits; digits += (maxDigits - 1);

	/* Handle number signs */
	Int32 offset = 0, i = 0;
	if (str->buffer[0] == '-') { *negative = true; offset = 1; }
	if (str->buffer[0] == '+') { offset = 1; }

	/* add digits, starting at last digit */
	for (i = str->length - 1; i >= offset; i--) {
		UInt8 c = str->buffer[i];
		if (c < '0' || c > '9' || digits < start) return false;
		*digits-- = c;
	}

	for (; digits >= start; ) { *digits-- = '0'; }
	return true;
}

bool Convert_TryParseInt32(STRING_PURE String* str, Int32* value) {
	*value = 0;
	#define INT32_DIGITS 10
	bool negative;	
	UInt8 digits[INT32_DIGITS];
	if (!Convert_TryParseDigits(str, &negative, digits, INT32_DIGITS)) return false;

	Int32 i, compare;
	if (negative) {
		compare = Convert_CompareDigits(digits, "2147483648");
		/* Special case, since |largest min value| is > |largest max value| */
		if (compare == 0) { *value = Int32_MinValue; return true; }
	} else {
		compare = Convert_CompareDigits(digits, "2147483647");
	}

	if (compare > 0) return false;	
	Int32 sum = 0;
	for (i = 0; i < INT32_DIGITS; i++) {
		sum *= 10; sum += digits[i] - '0';
	}

	if (negative) sum = -sum;
	*value = sum;
	return true;
}

bool Convert_TryParseInt64(STRING_PURE String* str, Int64* value) {
	*value = 0;
	#define INT64_DIGITS 19
	bool negative;
	UInt8 digits[INT64_DIGITS];
	if (!Convert_TryParseDigits(str, &negative, digits, INT64_DIGITS)) return false;

	Int32 i, compare;
	if (negative) {
		compare = Convert_CompareDigits(digits, "9223372036854775808");
		/* Special case, since |largest min value| is > |largest max value| */
		if (compare == 0) { *value = -9223372036854775807LL - 1LL; return true; }
	} else {
		compare = Convert_CompareDigits(digits, "9223372036854775807");
	}

	if (compare > 0) return false;
	Int64 sum = 0;
	for (i = 0; i < INT64_DIGITS; i++) {
		sum *= 10; sum += digits[i] - '0';
	}

	if (negative) sum = -sum;
	*value = sum;
	return true;
}

bool Convert_TryParseReal32(STRING_PURE String* str, Real32* value) {
	Int32 i = 0;
	*value = 0.0f;
	bool foundDecimalPoint = false;
	Real64 whole = 0.0f, fract = 0.0f, divide = 10.0f;

	/* Handle number signs */
	bool negate = false;
	if (str->length == 0) return false;
	if (str->buffer[0] == '-') { negate = true; i++; }
	if (str->buffer[0] == '+') { i++; }

	/* TODO: CHECK THIS WORKS!!! */
	for (; i < str->length; i++) {
		UInt8 c = str->buffer[i];
		/* Floating point number can have . in it */
		if (c == '.' || c == ',') {
			if (foundDecimalPoint) return false;
			foundDecimalPoint = true;
			continue;
		}

		if (c < '0' || c > '9') return false;
		Int32 digit = c - '0';
		if (!foundDecimalPoint) {
			whole *= 10; whole += digit;
		} else {
			fract += digit / divide; divide *= 10;
		}
	}

	Real64 sum = whole + fract;
	if (negate) sum = -sum;
	*value = (Real32)sum;
	return true;
}

bool Convert_TryParseBool(STRING_PURE String* str, bool* value) {
	if (String_CaselessEqualsConst(str, "True")) {
		*value = true; return true;
	}
	if (String_CaselessEqualsConst(str, "False")) {
		*value = false; return true;
	}
	*value = false; return false;
}

#define STRINGSBUFFER_LEN_SHIFT 9
#define STRINGSBUFFER_LEN_MASK  0x1FFUL
void StringsBuffer_Init(StringsBuffer* buffer) {
	StringsBuffer_UNSAFE_Reset(buffer);
	buffer->TextBuffer  = buffer->DefaultBuffer;
	buffer->FlagsBuffer = buffer->DefaultFlags;
	buffer->TextBufferElems  = STRINGSBUFFER_BUFFER_DEF_SIZE;
	buffer->FlagsBufferElems = STRINGSBUFFER_FLAGS_DEF_ELEMS;
}

void StringsBuffer_Free(StringsBuffer* buffer) {
	if (buffer->TextBuffer != buffer->DefaultBuffer) {
		Platform_MemFree(&buffer->TextBuffer);
	}
	if (buffer->FlagsBuffer != buffer->DefaultFlags) {
		Platform_MemFree(&buffer->FlagsBuffer);
	}
	StringsBuffer_UNSAFE_Reset(buffer);
}

void StringsBuffer_UNSAFE_Reset(StringsBuffer* buffer) {
	buffer->Count     = 0;
	buffer->UsedElems = 0;
}

void StringsBuffer_Get(StringsBuffer* buffer, UInt32 index, STRING_TRANSIENT String* text) {
	String raw = StringsBuffer_UNSAFE_Get(buffer, index);
	String_Clear(text);
	String_AppendString(text, &raw);
}

String StringsBuffer_UNSAFE_Get(StringsBuffer* buffer, UInt32 index) {
	if (index >= buffer->Count) ErrorHandler_Fail("Tried to get String past StringsBuffer end");

	UInt32 flags  = buffer->FlagsBuffer[index];
	UInt32 offset = flags >> STRINGSBUFFER_LEN_SHIFT;
	UInt32 len    = flags  & STRINGSBUFFER_LEN_MASK;
	return String_Init(&buffer->TextBuffer[offset], (UInt16)len, (UInt16)len);
}

void StringsBuffer_Resize(void** buffer, UInt32* elems, UInt32 elemSize, UInt32 defElems, UInt32 expandElems) {
	/* We use a statically allocated buffer initally, so can't realloc first time */
	void* dst;
	void* cur = *buffer;
	UInt32 curElems = *elems;

	if (curElems <= defElems) {
		dst = Platform_MemAlloc(curElems + expandElems, elemSize);
		if (dst == NULL) ErrorHandler_Fail("Failed allocating memory for StringsBuffer");
		Platform_MemCpy(dst, cur, curElems * elemSize);
	} else {
		dst = Platform_MemRealloc(cur, curElems + expandElems, elemSize);
		if (dst == NULL) ErrorHandler_Fail("Failed allocating memory for resizing StringsBuffer");
	}

	*buffer = dst;
	*elems  = curElems + expandElems;
}

void StringsBuffer_Add(StringsBuffer* buffer, STRING_PURE String* text) {
	if (buffer->Count == buffer->FlagsBufferElems) {
		/* Someone forgot to initalise flags buffer, abort */
		if (buffer->FlagsBufferElems == 0) {
			ErrorHandler_Fail("StringsBuffer not properly initalised");
		}
		StringsBuffer_Resize(&buffer->FlagsBuffer, &buffer->FlagsBufferElems, sizeof(UInt32),
			STRINGSBUFFER_FLAGS_DEF_ELEMS, STRINGSBUFFER_FLAGS_EXPAND_ELEMS);
	}

	if (text->length > STRINGSBUFFER_LEN_MASK) {
		ErrorHandler_Fail("String too big to insert into StringsBuffer");
	}

	UInt32 textOffset = buffer->UsedElems;
	if (textOffset + text->length >= buffer->TextBufferElems) {
		StringsBuffer_Resize(&buffer->TextBuffer, &buffer->TextBufferElems, sizeof(UInt8),
			STRINGSBUFFER_BUFFER_DEF_SIZE, STRINGSBUFFER_BUFFER_EXPAND_SIZE);
	}

	if (text->length > 0) {
		Platform_MemCpy(&buffer->TextBuffer[textOffset], text->buffer, text->length);
	}
	buffer->FlagsBuffer[buffer->Count] = text->length | (textOffset << STRINGSBUFFER_LEN_SHIFT);

	buffer->Count++;
	buffer->UsedElems += text->length;
}

void StringsBuffer_Remove(StringsBuffer* buffer, UInt32 index) {
	if (index >= buffer->Count) ErrorHandler_Fail("Tried to remove String past StringsBuffer end");

	UInt32 flags  = buffer->FlagsBuffer[index];
	UInt32 offset = flags >> STRINGSBUFFER_LEN_SHIFT;
	UInt32 len    = flags  & STRINGSBUFFER_LEN_MASK;

	/* Imagine buffer is this: AAXXYYZZ, and want to delete X */
	/* Start points to first character of Y */
	/* End points to character past last character of Z */
	UInt32 i, start = offset + len, end = buffer->UsedElems;
	for (i = start; i < end; i++) { 
		buffer->TextBuffer[i - len] = buffer->TextBuffer[i]; 
	}

	/* adjust text offset of elements after this element */
	/* Elements may not be in order so most account for that */
	UInt32 flagsLen = len << STRINGSBUFFER_LEN_SHIFT;
	for (i = index; i < buffer->Count; i++) {
		buffer->FlagsBuffer[i] = buffer->FlagsBuffer[i + 1];
		if (buffer->FlagsBuffer[i] >= flags) {
			buffer->FlagsBuffer[i] -= flagsLen;
		}
	}

	buffer->Count--;
	buffer->UsedElems -= len;
}

Int32 StringsBuffer_Compare(StringsBuffer* buffer, UInt32 idxA, UInt32 idxB) {
	String strA = StringsBuffer_UNSAFE_Get(buffer, idxA);
	String strB = StringsBuffer_UNSAFE_Get(buffer, idxB);
	return String_Compare(&strA, &strB);
}


bool WordWrap_IsWrapper(UInt8 c) {
	return c == NULL || c == ' ' || c == '-' || c == '>' || c == '<' || c == '/' || c == '\\';
}

void WordWrap_Do(STRING_REF String* text, STRING_TRANSIENT String* lines, Int32 numLines, Int32 lineLen) {
	Int32 i;
	for (i = 0; i < numLines; i++) { lines[i] = String_MakeNull(); }

	Int32 lineStart = 0, lineEnd;
	for (i = 0; i < numLines; i++) {
		Int32 nextLineStart = lineStart + lineLen;
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

/* Calculates where the given raw index is located in the wrapped lines. */
void WordWrap_GetCoords(Int32 index, STRING_PURE String* lines, Int32 numLines, Int32* coordX, Int32* coordY) {
	if (index == -1) index = Int32_MaxValue;
	Int32 offset = 0; *coordX = -1; *coordY = 0;

	Int32 y;
	for (y = 0; y < numLines; y++) {
		Int32 lineLength = lines[y].length;
		if (lineLength == 0) break;

		*coordY = y;
		if (index < offset + lineLength) {
			*coordX = index - offset; break;
		}
		offset += lineLength;
	}
	if (*coordX == -1) *coordX = lines[*coordY].length;
}

Int32 WordWrap_GetBackLength(STRING_PURE String* text, Int32 index) {
	if (index <= 0) return 0;
	if (index >= text->length) {
		ErrorHandler_Fail("WordWrap_GetBackLength - index past end of string");
	}

	Int32 start = index;
	bool lookingSpace = text->buffer[index] == ' ';
	/* go back to the end of the previous word */
	if (lookingSpace) {
		while (index > 0 && text->buffer[index] == ' ') { index--; }
	}

	/* go back to the start of the current word */
	while (index > 0 && text->buffer[index] != ' ') { index--; }
	return start - index;
}

Int32 WordWrap_GetForwardLength(STRING_PURE String* text, Int32 index) {
	if (index == -1) return 0;
	if (index >= text->length) {
		ErrorHandler_Fail("WordWrap_GetForwardLength - index past end of string");
	}

	Int32 start = index, length = text->length;
	bool lookingLetter = text->buffer[index] != ' ';
	/* go forward to the end of the current word */
	if (lookingLetter) {
		while (index < length && text->buffer[index] != ' ') { index++; }
	}

	/* go forward to the start of the next word */
	while (index < length && text->buffer[index] == ' ') { index++; }
	return index - start;
}