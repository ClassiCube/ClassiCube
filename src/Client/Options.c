#include "Options.h"
#include "ExtMath.h"
#include "ErrorHandler.h"
#include "Utils.h"
#include "Funcs.h"
#define OPT_NOT_FOUND UInt32_MaxValue

void Options_Init(void) {
	StringsBuffer_Init(&Options_Keys);
	StringsBuffer_Init(&Options_Values);
}

void Options_Free(void) {
	StringsBuffer_Free(&Options_Keys);
	StringsBuffer_Free(&Options_Values);
}

UInt32 Options_Find(STRING_PURE String* key) {
	UInt32 i;
	for (i = 0; i < Options_Keys.Count; i++) {
		String curKey = StringsBuffer_UNSAFE_Get(&Options_Keys, i);
		if (String_CaselessEquals(&curKey, key)) return i;
	}
	return OPT_NOT_FOUND;
}

bool Options_TryGetValue(const UInt8* keyRaw, STRING_TRANSIENT String* value) {
	String key = String_FromReadonly(keyRaw);
	*value = String_MakeNull();

	UInt32 i = Options_Find(&key);
	if (i != OPT_NOT_FOUND) {
		*value = StringsBuffer_UNSAFE_Get(&Options_Values, i);
		return true; 
	}

	Int32 sepIndex = String_IndexOf(&key, '-', 0);
	if (sepIndex == -1) return false;
	key = String_UNSAFE_SubstringAt(&key, sepIndex + 1);

	i = Options_Find(&key);
	if (i != OPT_NOT_FOUND) {
		*value = StringsBuffer_UNSAFE_Get(&Options_Values, i);
		return true;
	}
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

UInt32 Options_GetEnum(const UInt8* key, UInt32 defValue, const UInt8** names, UInt32 namesCount) {
	String str;
	if (!Options_TryGetValue(key, &str)) return defValue;
	return Utils_ParseEnum(&str, defValue, names, namesCount);
}

void Options_Remove(UInt32 i) {
	StringsBuffer_Remove(&Options_Keys, i);
	StringsBuffer_Remove(&Options_Values, i);
}

Int32 Options_Insert(STRING_PURE String* key, STRING_PURE String* value) {
	UInt32 i = Options_Find(key);
	if (i != OPT_NOT_FOUND) {
		Options_Remove(i);
		/* Reset Changed state for this option */
		for (; i < Array_NumElements(Options_Changed) - 1; i++) {
			Options_Changed[i] = Options_Changed[i + 1];
		}
	}

	StringsBuffer_Add(&Options_Keys, key);
	StringsBuffer_Add(&Options_Values, value);
	return Options_Keys.Count;
}

void Options_SetInt32(const UInt8* keyRaw, Int32 value) {
	UInt8 numBuffer[String_BufferSize(STRING_INT32CHARS)];
	String numStr = String_InitAndClearArray(numBuffer);
	String_AppendInt32(&numStr, value);
	Options_Set(keyRaw, &numStr);
}

void Options_Set(const UInt8* keyRaw, STRING_PURE String* value) {
	String key = String_FromReadonly(keyRaw);
	UInt32 i;
	if (value == NULL || value->buffer == NULL) {
		i = Options_Find(&key);
		if (i != OPT_NOT_FOUND) Options_Remove(i);
	} else {
		i = Options_Insert(&key, value);
	}
	if (i != OPT_NOT_FOUND) Options_Changed[i] = true;
}