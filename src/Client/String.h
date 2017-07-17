#ifndef CS_STRING_H
#define CS_STRING_H
#include "Typedefs.h"
/* Implements operations for a string.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

#define String_BufferSize(n) (n + 1)

typedef struct String_ {
	/* Pointer to raw characters. Size is capacity + 1, as buffer is null terminated. */
	UInt8* buffer;

	/* Number of characters used. */
	UInt16 length;

	/* Max number of characters that can be in buffer. */
	UInt16 capacity;
} String;

/* Constructs a new string, pointing a buffer consisting purely of NULL characters. */
String String_FromEmptyBuffer(UInt8* buffer, UInt16 capacity);

/* Constructs a new string, pointing a buffer consisting of arbitary data.
NOTE: This method sets the bytes occupied by the string to NUL. */
String String_FromRawBuffer(UInt8* buffer, UInt16 capacity);

/* Constructs a new string from a constant readonly string. */
String String_FromConstant(const UInt8* buffer);

/* Makes an empty string that points to nowhere. */
String String_MakeNull(void);

/* Sets all characters in the given string to lowercase. */
void String_MakeLowercase(String* str);


/* Returns whether two strings have same contents. */
bool String_Equals(String* a, String* b);

/* Returns whether two strings have same case-insensitive contents. */
bool String_CaselessEquals(String* a, String* b);


/* Attempts to append a character to the end of a string. */
bool String_Append(String* str, UInt8 c);

/* Attempts to append an integer value to the end of a string. */
bool String_AppendInt32(String* str, Int32 num);

/* Attempts to append an integer value to the end of a string, padding left with 0. */
bool String_AppendPaddedInt32(String* str, Int32 num, Int32 minDigits);

static Int32 String_MakeInt32(Int32 num, UInt8* numBuffer);

/* Attempts to append a constant raw null-terminated string. */
bool String_AppendConstant(String* str, const UInt8* buffer);

/* Attempts to append a string. */
bool String_AppendString(String* str, String* buffer);


/* Finds the first index of c in given string, -1 if not found. */
Int32 String_IndexOf(String* str, UInt8 c, Int32 offset);

/* Finds the last index of c in given string, -1 if not found. */
Int32 String_LastIndexOf(String* str, UInt8 c);

/* Gets the character at the given index in the string. */
UInt8 String_CharAt(String* str, Int32 offset);

#endif