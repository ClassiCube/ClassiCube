#include "String.h"
#include "Funcs.h"

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

String String_FromConstant(const UInt8* buffer) {
	UInt16 length = 0;
	UInt8 cur = 0;
	UInt8* ptr = buffer;

	while ((cur = *buffer) != 0) {
		length++; buffer++;
	}

	String str;
	str.buffer = ptr;
	str.capacity = length;
	str.length = length;
	return str;
}

String String_MakeNull(void) {
	String str;
	str.buffer = NULL;
	str.capacity = 0;
	str.length = 0;
	return str;
}

void String_MakeLowercase(String* str) {
	Int32 i;
	for (i = 0; i < str->length; i++) {
		str->buffer[i] = Char_ToLower(str->buffer[i]);
	}
}


bool String_Equals(String* a, String* b) {
	if (a->length != b->length) return false;
	Int32 i;

	for (i = 0; i < a->length; i++) {
		if (a->buffer[i] != b->buffer[i]) return false;
	}
	return true;
}

bool String_CaselessEquals(String* a, String* b) {
	if (a->length != b->length) return false;
	Int32 i;

	for (i = 0; i < a->length; i++) {
		UInt8 aCur = a->buffer[i];
		UInt8 bCur = b->buffer[i];

		if (Char_IsUpper(aCur)) aCur = Char_ToLower(aCur);
		if (Char_IsUpper(bCur)) bCur = Char_ToLower(bCur);

		if (aCur != bCur) return false;
	}
	return true;
}


bool String_Append(String* str, UInt8 c) {
	if (str->length == str->capacity) return false;

	str->buffer[str->length] = c;
	str->length++;
	return true;
}

bool String_AppendInt32(String* str, Int32 num) {
	if (num < 0) {
		num = -num;
		if (!String_Append(str, (UInt8)'-')) return false;
	}

	UInt8 numBuffer[20];
	Int32 numLen = String_MakeInt32(num, numBuffer);
	Int32 i;
	
	for (i = numLen - 1; i >= 0; i--) {
		if (!String_Append(str, numBuffer[i])) return false;
	}
	return true;
}

bool String_AppendPaddedInt32(String* str, Int32 num, Int32 minDigits) {
	UInt8 numBuffer[20];
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

static Int32 String_MakeInt32(Int32 num, UInt8* numBuffer) {
	Int32 len = 0;

	do {
		numBuffer[len] = (char)('0' + (num % 10)); num /= 10;
		len++; 
	} while (num > 0);
	return len;
}

bool String_AppendConstant(String* str, const UInt8* buffer) {
	UInt8 cur = 0;

	while ((cur = *buffer) != 0) {
		if (!String_Append(str, cur)) return false;
		buffer++;
	}
	return true;
}


Int32 String_IndexOf(String* str, UInt8 c, Int32 offset) {
	Int32 i;
	for (i = offset; i < str->length; i++) {
		if (str->buffer[i] == c) return i;
	}
	return -1;
}

Int32 String_LastIndexOf(String* str, UInt8 c) {
	Int32 i;
	for (i = str->length - 1; i >= 0; i--) {
		if (str->buffer[i] == c) return i;
	}
	return -1;
}

UInt8 String_CharAt(String* str, Int32 offset) {
	if (offset < 0 || offset >= str->length) return 0;
	return str->buffer[offset];
}