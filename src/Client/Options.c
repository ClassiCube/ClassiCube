#include "Options.h"
#include "ExtMath.h"
#include "ErrorHandler.h"

UInt8 Options_KeysBuffer[String_BufferSize(26) * OPTIONS_COUNT];
UInt8 Options_ValuesBuffer[
	String_BufferSize(8)   * OPTIONS_TINYSTRS +
	String_BufferSize(16)  * OPTIONS_SMALLSTRS +
	String_BufferSize(32)  * OPTIONS_MEDSTRS +
	String_BufferSize(512) * OPTIONS_LARGESTRS
];

#define Options_InitStrs(strArray, count, elemLen)\
for (i = 0; i < count; i++, j++) {\
	strArray[j] = String_FromEmptyBuffer(buffer, elemLen);\
	buffer += String_BufferSize(elemLen);\
}

void Options_Init(void) {
	Int32 i, j = 0;
	UInt8* buffer = Options_KeysBuffer;
	Options_InitStrs(Options_Keys, OPTIONS_COUNT, 24);

	j = 0;
	buffer = Options_ValuesBuffer;
	Options_InitStrs(Options_Values, OPTIONS_TINYSTRS,    8);
	Options_InitStrs(Options_Values, OPTIONS_SMALLSTRS,  16);
	Options_InitStrs(Options_Values, OPTIONS_MEDSTRS,    32);
	Options_InitStrs(Options_Values, OPTIONS_LARGESTRS, 512);
}

Int32 Options_Find(String key) {
	Int32 i;
	for (i = 0; i < OPTIONS_COUNT; i++) {
		if (String_CaselessEquals(&Options_Keys[i], &key)) return i;
	}
	return -1;
}

bool Options_TryGetValue(const UInt8* keyRaw, String* value) {
	String key = String_FromReadonly(keyRaw);
	*value = String_MakeNull();
	Int32 i = Options_Find(key);
	if (i >= 0) { *value = Options_Values[i]; return true; }

	Int32 sepIndex = String_IndexOf(&key, '-', 0);
	if (sepIndex == -1) return false;

	key = String_UNSAFE_SubstringAt(&key, sepIndex + 1);
	i = Options_Find(key);
	if (i >= 0) { *value = Options_Values[i]; return true; }
	return false;
}

String Options_Get(const UInt8* key) {
	String value;
	Options_TryGetValue(key, &value);
	return value;
}

Int32 Options_GetInt(const UInt8* key, Int32 min, Int32 max, Int32 defValue) {
	String str;
	Int32 value;
	if (!Options_TryGetValue(key, &str))      return defValue;
	if (!Convert_TryParseInt32(&str, &value)) return defValue;

	Math_Clamp(value, min, max);
	return value;
}

bool Options_GetBool(const UInt8* key, bool defValue) {
	String str;
	bool value;
	if (!Options_TryGetValue(key, &str))     return defValue;
	if (!Convert_TryParseBool(&str, &value)) return defValue;

	return value;
}

Real32 Options_GetFloat(const UInt8* key, Real32 min, Real32 max, Real32 defValue) {
	String str;
	Real32 value;
	if (!Options_TryGetValue(key, &str))      return defValue;
	if (!Convert_TryParseReal32(&str, &value)) return defValue;

	Math_Clamp(value, min, max);
	return value;
}

void Options_Remove(Int32 i) {
	String_Clear(&Options_Keys[i]);
	String_Clear(&Options_Values[i]);
}

Int32 Options_Insert(String key, String value) {
	Int32 i = Options_Find(key);
	/* The new value may not fit in the old slot, always insert into a new slot. */
	if (i >= 0) {
		Options_Remove(i);
		Options_Changed[i] = false;
	}

	for (i = 0; i < OPTIONS_COUNT; i++) {
		if (Options_Keys[i].length > 0) continue;
		if (Options_Values[i].capacity < value.length) continue;

		String_AppendString(&Options_Keys[i], &key);
		String_AppendString(&Options_Values[i], &value);
		return i;
	}

	ErrorHandler_Fail("No free slot left to save option");
	return -1;
}

void Options_SetInt32(const UInt8* keyRaw, Int32 value) {
	UInt8 numBuffer[String_BufferSize(STRING_INT32CHARS)];
	UInt8* ptr = numBuffer;
	String numStr = String_FromRawBuffer(ptr, STRING_INT32CHARS);
	String_AppendInt32(&numStr, value);
	Options_Set(keyRaw, numStr);
}

void Options_Set(const UInt8* keyRaw, STRING_TRANSIENT String value) {
	String key = String_FromReadonly(keyRaw);
	Int32 i;
	if (value.buffer == NULL) {
		i = Options_Find(key);
		if (i >= 0) Options_Remove(i);
	} else {
		i = Options_Insert(key, value);
	}
	if (i >= 0) Options_Changed[i] = true;
}