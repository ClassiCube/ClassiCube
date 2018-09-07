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
/* Indicates that a string argument is discarded after the function has completed, and is not modified. */
#define STRING_PURE
/* Indicates that a string argument is discarded after the function has completed, but **MAY BE MODIFIED**. */
#define STRING_TRANSIENT
/* Indicates that a reference to the buffer in a string argument is persisted after the function has completed.
Thus it is **NOT SAFE** to allocate a string on the stack. */
#define STRING_REF

char Char_ToLower(char c);

typedef struct String_ {	
	char* buffer;    /* Pointer to characters, NOT NULL TERMINATED */
	UInt16 length;   /* Number of characters used */
	UInt16 capacity; /* Max number of characters  */
} String;

UInt16 String_CalcLen(STRING_PURE const char* raw, UInt16 capacity);
String String_MakeNull(void);
String String_Init(STRING_REF char* buffer, UInt16 length, UInt16 capacity);
String String_InitAndClear(STRING_REF char* buffer, UInt16 capacity);
/* Constructs a new string from a (maybe null terminated) buffer. */
NOINLINE_ String String_FromRaw(STRING_REF char* buffer, UInt16 capacity);
/* Constructs a new string from a null-terminated constant readonly buffer. */
NOINLINE_ String String_FromReadonly(STRING_REF const char* buffer);

#define String_ClearedArray(buffer) String_InitAndClear(buffer, (UInt16)sizeof(buffer))
/* Constructs a new string from a compile time string constant. */
#define String_FromConst(text) { text, (UInt16)(sizeof(text) - 1), (UInt16)(sizeof(text) - 1)}
/* Constructs a new string from a compile time empty string buffer. */
#define String_FromArray(buffer) { buffer, 0, (UInt16)sizeof(buffer)}
/* Constructs a new string from a compile time array, that may have arbitary actual length of data at runtime */
#define String_FromRawArray(buffer) String_FromRaw(buffer, (UInt16)sizeof(buffer))

NOINLINE_ void String_StripCols(STRING_TRANSIENT String* str);
NOINLINE_ void String_Copy(STRING_TRANSIENT String* dst, STRING_PURE String* src);
NOINLINE_ String String_UNSAFE_Substring(STRING_REF String* str, Int32 offset, Int32 length);
#define String_UNSAFE_SubstringAt(str, offset) (String_UNSAFE_Substring(str, offset, (str)->length - (offset)))
NOINLINE_ void String_UNSAFE_Split(STRING_REF String* str, char c, STRING_TRANSIENT String* subs, Int32* subsCount);
NOINLINE_ bool String_UNSAFE_Separate(STRING_REF String* str, char c, STRING_TRANSIENT String* key, STRING_TRANSIENT String* value);

NOINLINE_ bool String_Equals(STRING_PURE String* a, STRING_PURE String* b);
NOINLINE_ bool String_CaselessEquals(STRING_PURE String* a, STRING_PURE String* b);
NOINLINE_ bool String_CaselessEqualsConst(STRING_PURE String* a, STRING_PURE const char* b);
NOINLINE_ Int32 String_MakeUInt32(UInt32 num, char* numBuffer);

bool String_Append(STRING_TRANSIENT String* str, char c);
bool String_AppendBool(STRING_TRANSIENT String* str, bool value);
bool String_AppendInt32(STRING_TRANSIENT String* str, Int32 num);
bool String_AppendUInt32(STRING_TRANSIENT String* str, UInt32 num);
bool String_AppendUInt64(STRING_TRANSIENT String* str, UInt64 num);
bool String_AppendReal32(STRING_TRANSIENT String* str, Real32 num, Int32 fracDigits); /* TODO: Need to account for , or . for decimal */
bool String_Hex32(STRING_TRANSIENT String* str, UInt32 value);
bool String_Hex64(STRING_TRANSIENT String* str, UInt64 value);
bool String_AppendConst(STRING_TRANSIENT String* str, const char* src);
bool String_AppendString(STRING_TRANSIENT String* str, STRING_PURE String* src);
bool String_AppendColorless(STRING_TRANSIENT String* str, STRING_PURE String* src);

NOINLINE_ Int32 String_IndexOf(STRING_PURE String* str, char c, Int32 offset);
NOINLINE_ Int32 String_LastIndexOf(STRING_PURE String* str, char c);
NOINLINE_ void String_InsertAt(STRING_TRANSIENT String* str, Int32 offset, char c);
NOINLINE_ void String_DeleteAt(STRING_TRANSIENT String* str, Int32 offset);
NOINLINE_ void String_UNSAFE_TrimStart(STRING_TRANSIENT String* str);
NOINLINE_ void String_UNSAFE_TrimEnd(STRING_TRANSIENT String* str);

NOINLINE_ Int32 String_IndexOfString(STRING_PURE String* str, STRING_PURE String* sub);
#define String_ContainsString(str, sub) (String_IndexOfString(str, sub) >= 0)
NOINLINE_ bool String_CaselessStarts(STRING_PURE String* str, STRING_PURE String* sub);
NOINLINE_ bool String_CaselessEnds(STRING_PURE String* str, STRING_PURE String* sub);
NOINLINE_ Int32 String_Compare(STRING_PURE String* a, STRING_PURE String* b);

void String_Format1(STRING_TRANSIENT String* str, const char* format, const void* a1);
void String_Format2(STRING_TRANSIENT String* str, const char* format, const void* a1, const void* a2);
void String_Format3(STRING_TRANSIENT String* str, const char* format, const void* a1, const void* a2, const void* a3);
void String_Format4(STRING_TRANSIENT String* str, const char* format, const void* a1, const void* a2, const void* a3, const void* a4);

UInt16 Convert_CP437ToUnicode(char c);
char Convert_UnicodeToCP437(UInt16 c);
bool Convert_TryUnicodeToCP437(UInt16 c, char* value);
void String_DecodeUtf8(STRING_TRANSIENT String* str, UInt8* data, UInt32 len);

NOINLINE_ bool Convert_TryParseUInt8(STRING_PURE String*  str, UInt8* value);
NOINLINE_ bool Convert_TryParseInt16(STRING_PURE String*  str, Int16* value);
NOINLINE_ bool Convert_TryParseUInt16(STRING_PURE String* str, UInt16* value);
NOINLINE_ bool Convert_TryParseInt32(STRING_PURE String*  str, Int32* value);
NOINLINE_ bool Convert_TryParseUInt64(STRING_PURE String* str, UInt64* value);
NOINLINE_ bool Convert_TryParseReal32(STRING_PURE String* str, Real32* value);
NOINLINE_ bool Convert_TryParseBool(STRING_PURE String*   str, bool* value);

#define STRINGSBUFFER_BUFFER_DEF_SIZE 4096
#define STRINGSBUFFER_FLAGS_DEF_ELEMS 256
typedef struct StringsBuffer_ {
	char*  TextBuffer;
	UInt32* FlagsBuffer;
	Int32 Count, TotalLength;
	/* internal state */
	Int32  _TextBufferSize, _FlagsBufferSize;
	char   _DefaultBuffer[STRINGSBUFFER_BUFFER_DEF_SIZE];
	UInt32 _DefaultFlags[STRINGSBUFFER_FLAGS_DEF_ELEMS];
} StringsBuffer;

NOINLINE_ void StringsBuffer_Init(StringsBuffer* buffer);
NOINLINE_ void StringsBuffer_Clear(StringsBuffer* buffer);
NOINLINE_ void StringsBuffer_Get(StringsBuffer* buffer, Int32 i, STRING_TRANSIENT String* text);
NOINLINE_ STRING_REF String StringsBuffer_UNSAFE_Get(StringsBuffer* buffer, Int32 i);
NOINLINE_ void StringsBuffer_Add(StringsBuffer* buffer, STRING_PURE String* text);
NOINLINE_ void StringsBuffer_Remove(StringsBuffer* buffer, Int32 index);

NOINLINE_ void  WordWrap_Do(STRING_REF String* text, STRING_TRANSIENT String* lines, Int32 numLines, Int32 lineLen);
NOINLINE_ void  WordWrap_GetCoords(Int32 index, STRING_PURE String* lines, Int32 numLines, Int32* coordX, Int32* coordY);
NOINLINE_ Int32 WordWrap_GetBackLength(STRING_PURE String* text, Int32 index);
NOINLINE_ Int32 WordWrap_GetForwardLength(STRING_PURE String* text, Int32 index);
#endif
