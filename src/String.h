#ifndef CC_STRING_H
#define CC_STRING_H
#include "Core.h"
/* Implements operations for a string.
   Also implements conversions betweens strings and numbers.
   Also implements converting code page 437 indices to/from unicode.
   Also implements wrapping a single line of text into multiple lines.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

#define STRING_INT_CHARS 24
/* Indicates that a reference to the buffer in a string argument is persisted after the function has completed.
Thus it is **NOT SAFE** to allocate a string on the stack. */
#define STRING_REF
/* Converts a character from A-Z to a-z, otherwise is left untouched. */
#define Char_MakeLower(c) if ((c) >= 'A' && (c) <= 'Z') { (c) += ' '; }

typedef struct String_ {	
	char* buffer;      /* Pointer to characters, NOT NULL TERMINATED */
	uint16_t length;   /* Number of characters used */
	uint16_t capacity; /* Max number of characters  */
} String;

/* Counts number of characters until a '\0' is found. */
int String_CalcLen(const char* raw, int capacity);
/* Constant string that points to NULL and has 0 length. */
/* NOTE: Do NOT modify the contents of this string! */
const String String_Empty;
/* Constructs a string from the given arguments. */
String String_Init(STRING_REF char* buffer, int length, int capacity);
/* Constructs a string from the given arguments, then sets all characters to '\0'. */
String String_InitAndClear(STRING_REF char* buffer, int capacity);
/* Constructs a string from a (maybe null terminated) buffer. */
NOINLINE_ String String_FromRaw(STRING_REF char* buffer, int capacity);
/* Constructs a string from a null-terminated constant readonly buffer. */
NOINLINE_ String String_FromReadonly(STRING_REF const char* buffer);

/* Constructs a string from a compile time array, then sets all characters to '\0'. */
#define String_ClearedArray(buffer) String_InitAndClear(buffer, sizeof(buffer))
/* Constructs a string from a compile time string constant */
#define String_FromConst(text) { text, (sizeof(text) - 1), (sizeof(text) - 1)}
/* Constructs a string from a compile time array */
#define String_FromArray(buffer) { buffer, 0, sizeof(buffer)}
/* Constructs a string from a compile time array, that may have arbitary actual length of data at runtime */
#define String_FromRawArray(buffer) String_FromRaw(buffer, sizeof(buffer))
/* Constructs a string from a compile time array (leaving 1 byte of room for null terminator) */
#define String_NT_Array(buffer) { buffer, 0, (sizeof(buffer) - 1)}

/* Removes all colour codes from the given string. */
NOINLINE_ void String_StripCols(String* str);
/* Sets length of dst to 0, then appends all characters in src. */
NOINLINE_ void String_Copy(String* dst, const String* src);
/* UNSAFE: Returns a substring of the given string. (sub.buffer is within str.buffer + str.length) */
NOINLINE_ String String_UNSAFE_Substring(STRING_REF const String* str, int offset, int length);
/* UNSAFE: Returns a substring of the given string. (sub.buffer is within str.buffer + str.length) */
#define String_UNSAFE_SubstringAt(str, offset) (String_UNSAFE_Substring(str, offset, (str)->length - (offset)))
NOINLINE_ void String_UNSAFE_Split(STRING_REF const String* str, char c, String* subs, int* subsCount);
/* UNSAFE: Splits a string of the form [key][c][value] into two substrings. */
/* e.g., allowed =true becomes 'allowed' and 'true', and excludes the space. */
NOINLINE_ bool String_UNSAFE_Separate(STRING_REF const String* str, char c, String* key, String* value);

/* Returns whether all characters of the strings are equal. */
NOINLINE_ bool String_Equals(const String* a, const String* b);
/* Returns whether all characters of the strings are case-insensitively equal. */
NOINLINE_ bool String_CaselessEquals(const String* a, const String* b);
/* Returns whether all characters of the strings are case-insensitively equal. */
/* NOTE: This is faster than String_CaselessEquals(a, String_FromReadonly(b)) */
NOINLINE_ bool String_CaselessEqualsConst(const String* a, const char* b);
/* Breaks down an integer into an array of digits. */
/* NOTE: Digits are in reverse order, so e.g. '200' becomes '0','0','2' */
NOINLINE_ int  String_MakeUInt32(uint32_t num, char* numBuffer);

/* Appends a character to the end of a string. */
/* Returns false when str->length == str->capcity, true otherwise. */
bool String_Append(String* str, char c);
/* Appends a boolean as either "true" or "false" to the end of a string. */
NOINLINE_ bool String_AppendBool(String* str, bool value);
/* Appends the digits of an integer (and -sign if negative) to the end of a string. */
NOINLINE_ bool String_AppendInt(String* str, int num);
/* Appends the digits of an unsigned 32 bit integer to the end of a string. */
NOINLINE_ bool String_AppendUInt32(String* str, uint32_t num);
/* Attempts to append an integer value to the end of a string, padding left with 0. */
NOINLINE_ bool String_AppendPaddedInt(String* str, int num, int minDigits);
/* Appends the digits of an unsigned 64 bit integer to the end of a string. */
NOINLINE_ bool String_AppendUInt64(String* str, uint64_t num);

/* Appends the digits of a float as a decimal. */
/* NOTE: If the number is an integer, no decimal point is added. */
/* Otherwise, fracDigits digits are added after a decimal point. */
/*  e.g. 1.0f produces "1", 2.6745f produces "2.67" when fracDigits is 2 */
NOINLINE_ bool String_AppendFloat(String* str, float num, int fracDigits); /* TODO: Need to account for , or . for decimal */
/* Appends characters to the end of a string. src MUST be null-terminated. */
NOINLINE_ bool String_AppendConst(String* str, const char* src);
/* Appends characters to the end of a string. */
NOINLINE_ bool String_AppendString(String* str, const String* src);
/* Appends characters to the end of a string, skipping any colour codes. */
NOINLINE_ bool String_AppendColorless(String* str, const String* src);
/* Appends the two hex digits of a byte to the end of a string. */
NOINLINE_ bool String_AppendHex(String* str, uint8_t value);

/* Returns first index of the given character in the given string, -1 if not found. */
NOINLINE_ int String_IndexOf(const String* str, char c, int offset);
/* Returns last index of the given character in the given string, -1 if not found. */
NOINLINE_ int String_LastIndexOf(const String* str, char c);
/* Inserts the given character into the given string. Exits process if this fails. */
/* e.g. inserting 'd' at offset '1' into "abc" produces "adbc" */
NOINLINE_ void String_InsertAt(String* str, int offset, char c);
/* Deletes a character from the given string. Exits process if this fails. */
/* e.g. deleting at offset '1' from "adbc" produces "abc" */
NOINLINE_ void String_DeleteAt(String* str, int offset);
/* Trims leading spaces from the given string. */
/* NOTE: Works by adjusting buffer/length, the characters in memory are not shifted. */
NOINLINE_ void String_TrimStart(String* str);
/* Trims trailing spaces from the given string. */
/* NOTE: Works by adjusting buffer/length, the characters in memory are not shifted. */
NOINLINE_ void String_TrimEnd(String* str);

/* Returns first index of the given substring in the given string, -1 if not found. */
/* e.g. index of "ab" within "cbabd" is 2 */
NOINLINE_ int String_IndexOfString(const String* str, const String* sub);
/* Returns whether given substring is inside the given string. */
#define String_ContainsString(str, sub) (String_IndexOfString(str, sub) >= 0)
/* Returns whether given substring is case-insensitively inside the given string. */
NOINLINE_ bool String_CaselessContains(const String* str, const String* sub);
/* Returns whether given substring is case-insensitively equal to the beginning of the given string. */
NOINLINE_ bool String_CaselessStarts(const String* str, const String* sub);
/* Returns whether given substring is case-insensitively equal to the ending of the given string. */
NOINLINE_ bool String_CaselessEnds(const String* str, const String* sub);
/* Compares the length of the given strings, then compares the characters if same length. Returns: */
/* -X if a.length < b.length, X if a.length > b.length */
/* -X if a.buffer[i] < b.buffer[i], X if a.buffer[i] > b.buffer[i] */
/* else returns 0. NOTE: The return value is not just in -1,0,1! */
NOINLINE_ int  String_Compare(const String* a, const String* b);

void String_Format1(String* str, const char* format, const void* a1);
void String_Format2(String* str, const char* format, const void* a1, const void* a2);
void String_Format3(String* str, const char* format, const void* a1, const void* a2, const void* a3);
void String_Format4(String* str, const char* format, const void* a1, const void* a2, const void* a3, const void* a4);

/* Converts a code page 437 character to its unicode equivalent. */
Codepoint Convert_CP437ToUnicode(char c);
/* Converts a unicode character to its code page 437 equivalent, or '?' if no match. */
char Convert_UnicodeToCP437(Codepoint cp);
/* Attempts to convert a unicode character to its code page 437 equivalent. */
bool Convert_TryUnicodeToCP437(Codepoint cp, char* value);
/* Appends all characters from UTF8 encoded data to the given string. */
void String_DecodeUtf8(String* str, uint8_t* data, uint32_t len);

/* Attempts to convert the given string into an unsigned 8 bit integer. */
NOINLINE_ bool Convert_TryParseUInt8(const String*  str, uint8_t* value);
/* Attempts to convert the given string into an signed 16 bit integer. */
NOINLINE_ bool Convert_TryParseInt16(const String*  str, int16_t* value);
/* Attempts to convert the given string into an unsigned 16 bit integer. */
NOINLINE_ bool Convert_TryParseUInt16(const String* str, uint16_t* value);
/* Attempts to convert the given string into an integer. */
NOINLINE_ bool Convert_TryParseInt(const String*    str, int* value);
/* Attempts to convert the given string into an unsigned 64 bit integer. */
NOINLINE_ bool Convert_TryParseUInt64(const String* str, uint64_t* value);
/* Attempts to convert the given string into a floating point number. */
NOINLINE_ bool Convert_TryParseFloat(const String*  str, float* value);
/* Attempts to convert the given string into a bool. */
/* NOTE: String must case-insensitively equal "true" or "false" */
NOINLINE_ bool Convert_TryParseBool(const String*   str, bool* value);

#define STRINGSBUFFER_BUFFER_DEF_SIZE 4096
#define STRINGSBUFFER_FLAGS_DEF_ELEMS 256
typedef struct StringsBuffer_ {
	char*      TextBuffer;  /* Raw characters of all entries */
	uint32_t*  FlagsBuffer; /* Private flags for each entry */
	int Count, TotalLength;
	/* internal state */
	int      _TextBufferSize, _FlagsBufferSize;
	char     _DefaultBuffer[STRINGSBUFFER_BUFFER_DEF_SIZE];
	uint32_t _DefaultFlags[STRINGSBUFFER_FLAGS_DEF_ELEMS];
} StringsBuffer;

/* Resets counts to 0, and frees any allocated memory. */
NOINLINE_ void StringsBuffer_Clear(StringsBuffer* buffer);
/* Copies the characters from the i'th string in the given buffer into the given string. */
NOINLINE_ void StringsBuffer_Get(StringsBuffer* buffer, int i, String* text);
/* UNSAFE: Returns a direct pointer to the i'th string in the given buffer. */
NOINLINE_ STRING_REF String StringsBuffer_UNSAFE_Get(StringsBuffer* buffer, int i);
/* Adds a given string to the end of the given buffer. */
NOINLINE_ void StringsBuffer_Add(StringsBuffer* buffer, const String* text);
/* Removes the i'th string from the given buffer, shifting following strings downwards. */
NOINLINE_ void StringsBuffer_Remove(StringsBuffer* buffer, int index);

/* Performs line wrapping on the given string. */
/* e.g. "some random tex|t* (| is lineLen) becomes "some random" "text" */
NOINLINE_ void WordWrap_Do(STRING_REF String* text, String* lines, int numLines, int lineLen);
/* Calculates the position of a raw index in the wrapped lines. */
NOINLINE_ void WordWrap_GetCoords(int index, const String* lines, int numLines, int* coordX, int* coordY);
/* Returns number of characters from current character to end of previous word. */
NOINLINE_ int  WordWrap_GetBackLength(const String* text, int index);
/* Returns number of characters from current character to start of next word. */
NOINLINE_ int  WordWrap_GetForwardLength(const String* text, int index);
#endif
