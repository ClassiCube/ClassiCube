#include "Options.h"
#include "ExtMath.h"
#include "ErrorHandler.h"
#include "Utils.h"
#include "Funcs.h"
#include "Platform.h"
#include "Stream.h"

const UInt8* FpsLimit_Names[FpsLimit_Count] = {
	"LimitVSync", "Limit30FPS", "Limit60FPS", "Limit120FPS", "LimitNone",
};
#define OPT_NOT_FOUND UInt32_MaxValue
StringsBuffer Options_Changed;

bool Options_HasAnyChanged(void) { return Options_Changed.Count > 0;  }

void Options_Init(void) {
	StringsBuffer_Init(&Options_Keys);
	StringsBuffer_Init(&Options_Values);
	StringsBuffer_Init(&Options_Changed);
}

void Options_Free(void) {
	StringsBuffer_Free(&Options_Keys);
	StringsBuffer_Free(&Options_Values);
	StringsBuffer_Free(&Options_Changed);
}

bool Options_HasChanged(STRING_PURE String* key) {
	UInt32 i;
	for (i = 0; i < Options_Changed.Count; i++) {
		String curKey = StringsBuffer_UNSAFE_Get(&Options_Changed, i);
		if (String_CaselessEquals(&curKey, key)) return true;
	}
	return false;
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

void Options_Get(const UInt8* key, STRING_TRANSIENT String* value, const UInt8* defValue) {
	String str;
	Options_TryGetValue(key, &str);
	String_Clear(value);

	if (str.length > 0) {
		String_AppendString(value, &str);
	} else {
		String_AppendConst(value, defValue);
	}
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
	if (i != OPT_NOT_FOUND) Options_Remove(i);

	StringsBuffer_Add(&Options_Keys, key);
	StringsBuffer_Add(&Options_Values, value);
	return Options_Keys.Count;
}

void Options_SetBool(const UInt8* keyRaw, bool value) {
	if (value) {
		String str = String_FromConst("True");  Options_Set(keyRaw, &str);
	} else {
		String str = String_FromConst("False"); Options_Set(keyRaw, &str);
	}
}

void Options_SetInt32(const UInt8* keyRaw, Int32 value) {
	UInt8 numBuffer[String_BufferSize(STRING_INT_CHARS)];
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

	if (i == OPT_NOT_FOUND || Options_HasChanged(&key)) return;
	StringsBuffer_Add(&Options_Changed, &key);
}

void Options_Load(void) {
	void* file;
	String path = String_FromConst("options.txt");
	ReturnCode result = Platform_FileOpen(&file, &path);

	if (result == ReturnCode_FileNotFound) return;
	/* TODO: Should we just log failure to open? */
	ErrorHandler_CheckOrFail(result, "Options - Loading");

	UInt8 lineBuffer[String_BufferSize(2048)];
	String line = String_InitAndClearArray(lineBuffer);
	Stream stream; Stream_FromFile(&stream, file, &path);

	/* Remove all the unchanged options */
	UInt32 i;
	for (i = Options_Keys.Count; i > 0; i--) {
		String key = StringsBuffer_UNSAFE_Get(&Options_Keys, i - 1);
		if (Options_HasChanged(&key)) continue;
		Options_Remove(i - 1);
	}

	while (Stream_ReadLine(&stream, &line)) {
		if (line.length == 0 || line.buffer[0] == '#') continue;

		Int32 sepIndex = String_IndexOf(&line, '=', 0);
		if (sepIndex <= 0) continue;
		String key = String_UNSAFE_Substring(&line, 0, sepIndex);

		sepIndex++;
		if (sepIndex == line.length) continue;
		String value = String_UNSAFE_SubstringAt(&line, sepIndex);

		if (!Options_HasChanged(&key)) {
			Options_Insert(&key, &value);
		}
	}

	result = stream.Close(&stream);
	ErrorHandler_CheckOrFail(result, "Options load - close file");
}

void Options_Save(void) {
	void* file;
	String path = String_FromConst("options.txt");
	ReturnCode result = Platform_FileCreate(&file, &path);

	/* TODO: Should we just log failure to save? */
	ErrorHandler_CheckOrFail(result, "Options - Saving");

	UInt8 lineBuffer[String_BufferSize(2048)];
	String line = String_InitAndClearArray(lineBuffer);
	Stream stream; Stream_FromFile(&stream, file, &path);
	UInt32 i;

	for (i = 0; i < Options_Keys.Count; i++) {
		String key   = StringsBuffer_UNSAFE_Get(&Options_Keys,   i);
		String value = StringsBuffer_UNSAFE_Get(&Options_Values, i);
		String_Format2(&line, "%s=%s", &key, &value);

		Stream_WriteLine(&stream, &line);
		String_Clear(&line);
	}

	result = stream.Close(&stream);
	ErrorHandler_CheckOrFail(result, "Options save - close file");
	StringsBuffer_Free(&Options_Changed);
}