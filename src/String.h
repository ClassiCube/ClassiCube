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

char Char_ToLower(char c);

typedef struct String_ {	
	char* buffer;    /* Pointer to characters, NOT NULL TERMINATED */
	UInt16 length;   /* Number of characters used */
	UInt16 capacity; /* Max number of characters  */
} String;

UInt16 String_CalcLen(const char* raw, UInt16 capacity);
String String_MakeNull(void);
String String_Init(STRING_REF char* buffer, UInt16 length, UInt16 capacity);
String String_InitAndClear(STRING_REF char* buffer, UInt16 capacity);
/* Constructs a new string from a (maybe null terminated) buffer. */
NOINLINE_ String String_FromRaw(STRING_REF char* buffer, UInt16 capacity);
/* Constructs a new string from a null-terminated constant readonly buffer. */
NOINLINE_ String String_FromReadonly(STRING_REF const char* buffer);

#define String_ClearedArray(buffer) String_InitAndClear(buffer, (UInt16)sizeof(buffer))
/* Constructs a string from a compile time string constant */
#define String_FromConst(text) { text, (UInt16)(sizeof(text) - 1), (UInt16)(sizeof(text) - 1)}
/* Constructs a string from a compile time array */
#define String_FromArray(buffer) { buffer, 0, (UInt16)sizeof(buffer)}
/* Constructs a string from a compile time array, that may have arbitary actual length of data at runtime */
#define String_FromRawArray(buffer) String_FromRaw(buffer, (UInt16)sizeof(buffer))
/* Constructs a string from a compile time array (leaving 1 byte of room for null terminator) */
#define String_NT_Array(buffer) { buffer, 0, (UInt16)(sizeof(buffer) - 1)}

NOINLINE_ void String_StripCols(String* str);
NOINLINE_ void String_Copy(String* dst, const String* src);
NOINLINE_ String String_UNSAFE_Substring(STRING_REF const String* str, int offset, int length);
#define String_UNSAFE_SubstringAt(str, offset) (String_UNSAFE_Substring(str, offset, (str)->length - (offset)))
NOINLINE_ void String_UNSAFE_Split(STRING_REF const String* str, char c, String* subs, int* subsCount);
NOINLINE_ bool String_UNSAFE_Separate(STRING_REF const String* str, char c, String* key, String* value);

NOINLINE_ bool String_Equals(const String* a, const String* b);
NOINLINE_ bool String_CaselessEquals(const String* a, const String* b);
NOINLINE_ bool String_CaselessEqualsConst(const String* a, const char* b);
NOINLINE_ Int32 String_MakeUInt32(UInt32 num, char* numBuffer);

bool String_Append(String* str, char c);
NOINLINE_ bool String_AppendBool(String* str, bool value);
NOINLINE_ bool String_AppendInt32(String* str, Int32 num);
NOINLINE_ bool String_AppendUInt32(String* str, UInt32 num);
NOINLINE_ bool String_AppendUInt64(String* str, UInt64 num);
NOINLINE_ bool String_AppendReal32(String* str, float num, Int32 fracDigits); /* TODO: Need to account for , or . for decimal */
NOINLINE_ bool String_AppendConst(String* str, const char* src);
NOINLINE_ bool String_AppendString(String* str, const String* src);
NOINLINE_ bool String_AppendColorless(String* str, const String* src);
NOINLINE_ bool String_AppendHex(String* str, UInt8 value);

NOINLINE_ int String_IndexOf(const String* str, char c, int offset);
NOINLINE_ int String_LastIndexOf(const String* str, char c);
NOINLINE_ void String_InsertAt(String* str, int offset, char c);
NOINLINE_ void String_DeleteAt(String* str, int offset);
NOINLINE_ void String_TrimStart(String* str);
NOINLINE_ void String_TrimEnd(String* str);

NOINLINE_ int String_IndexOfString(const String* str, const String* sub);
#define String_ContainsString(str, sub) (String_IndexOfString(str, sub) >= 0)
NOINLINE_ bool String_CaselessContains(const String* str, const String* sub);
NOINLINE_ bool String_CaselessStarts(const String* str, const String* sub);
NOINLINE_ bool String_CaselessEnds(const String* str, const String* sub);
NOINLINE_ int  String_Compare(const String* a, const String* b);

void String_Format1(String* str, const char* format, const void* a1);
void String_Format2(String* str, const char* format, const void* a1, const void* a2);
void String_Format3(String* str, const char* format, const void* a1, const void* a2, const void* a3);
void String_Format4(String* str, const char* format, const void* a1, const void* a2, const void* a3, const void* a4);

UInt16 Convert_CP437ToUnicode(char c);
char Convert_UnicodeToCP437(UInt16 c);
bool Convert_TryUnicodeToCP437(UInt16 c, char* value);
void String_DecodeUtf8(String* str, UInt8* data, UInt32 len);

NOINLINE_ bool Convert_TryParseUInt8(const String*  str, UInt8* value);
NOINLINE_ bool Convert_TryParseInt16(const String*  str, Int16* value);
NOINLINE_ bool Convert_TryParseUInt16(const String* str, UInt16* value);
NOINLINE_ bool Convert_TryParseInt32(const String*  str, Int32* value);
NOINLINE_ bool Convert_TryParseUInt64(const String* str, UInt64* value);
NOINLINE_ bool Convert_TryParseFloat(const String* str, float* value);
NOINLINE_ bool Convert_TryParseBool(const String*   str, bool* value);

#define STRINGSBUFFER_BUFFER_DEF_SIZE 4096
#define STRINGSBUFFER_FLAGS_DEF_ELEMS 256
typedef struct StringsBuffer_ {
	char*  TextBuffer;
	UInt32* FlagsBuffer;
	int Count, TotalLength;
	/* internal state */
	int    _TextBufferSize, _FlagsBufferSize;
	char   _DefaultBuffer[STRINGSBUFFER_BUFFER_DEF_SIZE];
	UInt32 _DefaultFlags[STRINGSBUFFER_FLAGS_DEF_ELEMS];
} StringsBuffer;

NOINLINE_ void StringsBuffer_Init(StringsBuffer* buffer);
NOINLINE_ void StringsBuffer_Clear(StringsBuffer* buffer);
NOINLINE_ void StringsBuffer_Get(StringsBuffer* buffer, int i, String* text);
NOINLINE_ STRING_REF String StringsBuffer_UNSAFE_Get(StringsBuffer* buffer, int i);
NOINLINE_ void StringsBuffer_Add(StringsBuffer* buffer, const String* text);
NOINLINE_ void StringsBuffer_Remove(StringsBuffer* buffer, int index);

NOINLINE_ void WordWrap_Do(STRING_REF String* text, String* lines, int numLines, int lineLen);
NOINLINE_ void WordWrap_GetCoords(int index, const String* lines, int numLines, int* coordX, int* coordY);
NOINLINE_ int  WordWrap_GetBackLength(const String* text, int index);
NOINLINE_ int  WordWrap_GetForwardLength(const String* text, int index);
#endif
