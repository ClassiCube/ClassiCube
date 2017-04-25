#include "String.h"

void String_Empty(String* str, UInt8* buffer, Int16 capacity) {
	str->buffer = buffer;
	str->capacity = capacity;
	str->length = 0;
}

void String_Constant(String* str, const UInt8* buffer) {
	Int16 length = 0;
	UInt8 cur = 0;

	while ((cur = *buffer) != 0) {
		length++; buffer++;
	}

	str->buffer = buffer;
	str->capacity = length;
	str->length = length;
}

bool String_Equals(String* a, String* b) {
	if (a->length != b->length) return false;

	for (Int32 i = 0; i < a->length; i++) {
		if (a->buffer[i] != b->buffer[i]) return false;
	}
	return true;
}

int String_IndexOf(String* str, UInt8 c) {
	for (Int32 i = 0; i < str->length; i++) {
		if (str->buffer[i] == c) return i;
	}
	return -1;
}