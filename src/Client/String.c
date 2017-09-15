#include "String.h"
#include "Funcs.h"
#include "ErrorHandler.h"

String String_FromEmptyBuffer(UInt8* buffer, UInt16 capacity) {
	String str;
	str.buffer = buffer;
	str.capacity = capacity;
	str.length = 0;
	return str;
}

String String_FromRawBuffer(UInt8* buffer, UInt16 capacity) {
	String str = String_FromEmptyBuffer(buffer, capacity);	
	Int32 i;

	/* Need to set region occupied by string to NUL for interop with native APIs */
	for (i = 0; i < capacity + 1; i++) {
		buffer[i] = 0;
	}
	return str;
}

String String_FromReadonly(const UInt8* buffer) {
	UInt16 length = 0;
	UInt8 cur = 0;
	UInt8* ptr = buffer;

	while ((cur = *buffer) != 0) {
		length++; buffer++;
	}

	String str = String_FromEmptyBuffer(ptr, length);
	str.length = length;
	return str;
}

String String_MakeNull(void) {
	return String_FromEmptyBuffer(NULL, 0);
}


void String_MakeLowercase(STRING_TRANSIENT String* str) {
	Int32 i;
	for (i = 0; i < str->length; i++) {
		str->buffer[i] = Char_ToLower(str->buffer[i]);
	}
}

void String_Clear(STRING_TRANSIENT String* str) {
	Int32 i;
	for (i = 0; i < str->length; i++) {
		str->buffer[i] = 0;
	}
	str->length = 0;
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

	String sub = *str;
	sub.buffer += offset;
	sub.length = length; 
	sub.capacity = length;
	return sub;
}


bool String_Equals(STRING_TRANSIENT String* a, STRING_TRANSIENT String* b) {
	if (a->length != b->length) return false;
	Int32 i;

	for (i = 0; i < a->length; i++) {
		if (a->buffer[i] != b->buffer[i]) return false;
	}
	return true;
}

bool String_CaselessEquals(STRING_TRANSIENT String* a, STRING_TRANSIENT String* b) {
	if (a->length != b->length) return false;
	Int32 i;

	for (i = 0; i < a->length; i++) {
		UInt8 aCur = Char_ToLower(a->buffer[i]);
		UInt8 bCur = Char_ToLower(b->buffer[i]);
		if (aCur != bCur) return false;
	}
	return true;
}


bool String_Append(STRING_TRANSIENT String* str, UInt8 c) {
	if (str->length == str->capacity) return false;

	str->buffer[str->length] = c;
	str->length++;
	return true;
}

Int32 String_MakeInt32(Int32 num, UInt8* numBuffer) {
	Int32 len = 0;

	do {
		numBuffer[len] = (UInt8)('0' + (num % 10));
		num /= 10; len++;
	} while (num > 0);
	return len;
}

bool String_AppendInt32(STRING_TRANSIENT String* str, Int32 num) {
	if (num < 0) {
		num = -num;
		if (!String_Append(str, (UInt8)'-')) return false;
	}

	UInt8 numBuffer[STRING_INT32CHARS];
	Int32 numLen = String_MakeInt32(num, numBuffer);
	Int32 i;
	
	for (i = numLen - 1; i >= 0; i--) {
		if (!String_Append(str, numBuffer[i])) return false;
	}
	return true;
}

bool String_AppendPaddedInt32(STRING_TRANSIENT String* str, Int32 num, Int32 minDigits) {
	UInt8 numBuffer[STRING_INT32CHARS];
	Int32 i;
	for (i = 0; i < minDigits; i++) {
		numBuffer[i] = '0';
	}

	Int32 numLen = String_MakeInt32(num, numBuffer);
	if (numLen < minDigits) numLen = minDigits;

	for (i = numLen - 1; i >= 0; i--) {
		if (!String_Append(str, numBuffer[i])) return false;
	}
	return true;
}

bool String_AppendConstant(STRING_TRANSIENT String* str, const UInt8* buffer) {
	UInt8 cur = 0;

	while ((cur = *buffer) != 0) {
		if (!String_Append(str, cur)) return false;
		buffer++;
	}
	return true;
}

bool String_AppendString(STRING_TRANSIENT String* str, String* buffer) {
	Int32 i;

	for (i = 0; i < buffer->length; i++) {
		if (!String_Append(str, buffer->buffer[i])) return false;
	}
	return true;
}


Int32 String_IndexOf(STRING_TRANSIENT String* str, UInt8 c, Int32 offset) {
	Int32 i;
	for (i = offset; i < str->length; i++) {
		if (str->buffer[i] == c) return i;
	}
	return -1;
}

Int32 String_LastIndexOf(STRING_TRANSIENT String* str, UInt8 c) {
	Int32 i;
	for (i = str->length - 1; i >= 0; i--) {
		if (str->buffer[i] == c) return i;
	}
	return -1;
}

UInt8 String_CharAt(STRING_TRANSIENT String* str, Int32 offset) {
	if (offset < 0 || offset >= str->length) return 0;
	return str->buffer[offset];
}

Int32 String_IndexOfString(STRING_TRANSIENT String* str, STRING_TRANSIENT String* sub) {
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


#define Convert_ControlCharsCount 32
UInt16 Convert_ControlChars[Convert_ControlCharsCount] = {
	0x0000, 0x263A, 0x263B, 0x2665, 0x2666, 0x2663, 0x2660, 0x2022,
	0x25D8, 0x25CB, 0x25D9, 0x2642, 0x2640, 0x266A, 0x266B, 0x263C,
	0x25BA, 0x25C4, 0x2195, 0x203C, 0x00B6, 0x00A7, 0x25AC, 0x21A8,
	0x2191, 0x2193, 0x2192, 0x2190, 0x221F, 0x2194, 0x25B2, 0x25BC
};

#define Convert_ExtendedCharsCount 129
UInt16 Convert_ExtendedChars[Convert_ExtendedCharsCount] = { 0x2302,
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
	if (c >= 0x20 && c < 0x7F) return (UInt8)c;
	UInt32 i;

	for (i = 0; i < Convert_ControlCharsCount; i++) {
		if (Convert_ControlChars[i] == c) return (UInt8)i;
	}
	for (i = 0; i < Convert_ExtendedCharsCount; i++) {
		if (Convert_ExtendedChars[i] == c) return (UInt8)(i + 0x7F);
	}
	return (UInt8)'?';
}

bool Convert_TryParseInt32(STRING_TRANSIENT String* str, Int32* value) {
	Int32 sum = 0, i = 0;
	*value = 0;

	/* Handle number signs */
	bool negate = false;
	if (str->length == 0) return false;
	if (str->buffer[0] == '-') { negate = true; i++; }
	if (str->buffer[0] == '+') { i++; }

	/* TODO: CHECK THIS WORKS!!! */
	for (; i < str->length; i++) {
		UInt8 c = str->buffer[i];
		if (c < '0' || c > '9') return false;
		Int32 digit = c - '0';

		/* Magnitude of largest negative integer cannot be expressed
		as a positive integer, so this case must be specially handled. */
		if (sum == (Int32)214748364 && digit == 8 && negate) {
			*value = Int32_MinValue;
			return true;
		}

		/* Overflow handling */
		if (sum >= (Int32)214748364) {
			Int32 diff = sum - (Int32)214748364;
			diff *= 10; diff += digit;

			/* Handle magnitude of max negative value specially,
			as it cannot be represented as a positive integer */
			if (diff == 8 && negate) {
				*value = Int32_MinValue;
				return true;
			}

			/* Overflows max positive value */
			if (diff > 7) return false;
		}
		sum *= 10; sum += digit;
	}

	if (negate) sum = -sum;
	*value = sum;
	return true;
}

bool Convert_TryParseUInt8(STRING_TRANSIENT String* str, UInt8* value) {
	*value = 0; Int32 tmp;
	if (!Convert_TryParseInt32(str, &tmp) || tmp < 0 || tmp > UInt8_MaxValue) return false;
	*value = (UInt8)tmp; return true;
}

bool Convert_TryParseUInt16(STRING_TRANSIENT String* str, UInt16* value) {
	*value = 0; Int32 tmp;
	if (!Convert_TryParseInt32(str, &tmp) || tmp < 0 || tmp > UInt16_MaxValue) return false;
	*value = (UInt16)tmp; return true;
}

bool Convert_TryParseReal32(STRING_TRANSIENT String* str, Real32* value) {
	Int32 i = 0;
	*value = 0.0f;
	bool foundDecimalPoint = false;
	Real32 whole = 0.0f, fract = 0.0f, divide = 10.0f;

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

	Real32 sum = whole + fract;
	if (negate) sum = -sum;
	*value = sum;
	return true;
}

bool Convert_TryParseBool(STRING_TRANSIENT String* str, bool* value) {
	String trueStr  = String_FromConstant("true");
	if (String_CaselessEquals(str, &trueStr)) {
		*value = true; return true;
	}

	String falseStr = String_FromConstant("false");
	if (String_CaselessEquals(str, &falseStr)) {
		*value = false; return true;
	}

	*value = false; return false;
}