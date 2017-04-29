#include "String.h"
#include "Funcs.h"

String String_FromBuffer(UInt8* buffer, UInt16 capacity) {
	String str;
	str.buffer = buffer;
	str.capacity = capacity;
	str.length = 0;
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

bool String_Equals(String* a, String* b) {
	if (a->length != b->length) return false;

	for (Int32 i = 0; i < a->length; i++) {
		if (a->buffer[i] != b->buffer[i]) return false;
	}
	return true;
}

bool String_CaselessEquals(String* a, String* b) {
	if (a->length != b->length) return false;

	for (Int32 i = 0; i < a->length; i++) {
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

Int32 String_IndexOf(String* str, UInt8 c, Int32 offset) {
	for (Int32 i = offset; i < str->length; i++) {
		if (str->buffer[i] == c) return i;
	}
	return -1;
}

UInt8 String_CharAt(String* str, Int32 offset) {
	if (offset < 0 || offset >= str->length) return 0;
	return str->buffer[offset];
}