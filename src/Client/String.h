#ifndef CS_STRING_H
#define CS_STRING_H
#include "Typedefs.h"
/* Implements operations for a string.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

#define String_BufferSize(n) (n + 1)

typedef struct String {
	/* Pointer to raw characters. Size is capacity + 1, as buffer is null terminated. */
	UInt8* buffer;

	/* Number of characters used. */
	UInt16 length;

	/* Max number of characters that can be in buffer. */
	UInt16 capacity;
} String;

/* Constructs a new string, filled with NULL characters. */
String String_FromBuffer(UInt8* buffer, UInt16 capacity);

/* Constructs a new string from a constant readonly string. */
String String_FromConstant(const UInt8* buffer);

/* Returns whether two strings have same contents. */
bool String_Equals(String* a, String* b);

/* Returns whether two strings have same case-insensitive contents. */
bool String_CaselessEquals(String* a, String* b);

/* Attempts to append a character to the end of a string. */
bool String_Append(String* str, UInt8 c);

/* Finds the first index of c in given string, -1 if not found. */
int String_IndexOf(String* str, UInt8 c, Int32 offset);

/* Gets the character at the given index in the string. */
UInt8 String_CharAt(String* str, Int32 offset);

#endif