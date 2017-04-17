#ifndef CS_STRING_H
#define CS_STRING_H
#include "Typedefs.h"
/* Implements operations for a string.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

typedef struct String {
	/* Pointer to raw characters. Size is capacity + 1, as buffer is null terminated. */
	UInt8* buffer;

	/* Number of characters used. */
	Int16 length;

	/* Max number of characters that can be in buffer. */
	Int16 capacity;
} String;

/* Constructs a new string, filled with NULL characters. */
void String_Empty(String* str, UInt8* buffer, Int16 capacity);

/* Constructs a new string from a constant readonly string. */
void String_Constant(String* str, const UInt8* buffer);

/* Returns whether two strings have same contents. */
bool String_Equals(String* a, String* b);

/* Finds the first index of c in given string, -1 if not found. */
int String_IndexOf(String* str, UInt8 c);

#endif