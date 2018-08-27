#include "Options.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Platform.h"
#include "Stream.h"
#include "Chat.h"
#include "Errors.h"

const char* FpsLimit_Names[FpsLimit_Count] = {
	"LimitVSync", "Limit30FPS", "Limit60FPS", "Limit120FPS", "LimitNone",
};
#define OPT_NOT_FOUND UInt32_MaxValue
StringsBuffer Options_Changed;

bool Options_HasAnyChanged(void) { return Options_Changed.Count > 0;  }

void Options_Free(void) {
	StringsBuffer_Clear(&Options_Keys);
	StringsBuffer_Clear(&Options_Values);
	StringsBuffer_Clear(&Options_Changed);
}

bool Options_HasChanged(STRING_PURE String* key) {
	Int32 i;
	for (i = 0; i < Options_Changed.Count; i++) {
		String curKey = StringsBuffer_UNSAFE_Get(&Options_Changed, i);
		if (String_CaselessEquals(&curKey, key)) return true;
	}
	return false;
}

static UInt32 Options_Find(STRING_PURE String* key) {
	Int32 i;
	for (i = 0; i < Options_Keys.Count; i++) {
		String curKey = StringsBuffer_UNSAFE_Get(&Options_Keys, i);
		if (String_CaselessEquals(&curKey, key)) return i;
	}
	return OPT_NOT_FOUND;
}

static bool Options_TryGetValue(const char* keyRaw, STRING_TRANSIENT String* value) {
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

void Options_Get(const char* key, STRING_TRANSIENT String* value, const char* defValue) {
	String str;
	Options_TryGetValue(key, &str);
	value->length = 0;

	if (str.length) {
		String_AppendString(value, &str);
	} else {
		String_AppendConst(value, defValue);
	}
}

Int32 Options_GetInt(const char* key, Int32 min, Int32 max, Int32 defValue) {
	String str;
	Int32 value;
	if (!Options_TryGetValue(key, &str))      return defValue;
	if (!Convert_TryParseInt32(&str, &value)) return defValue;

	Math_Clamp(value, min, max);
	return value;
}

bool Options_GetBool(const char* key, bool defValue) {
	String str;
	bool value;
	if (!Options_TryGetValue(key, &str))     return defValue;
	if (!Convert_TryParseBool(&str, &value)) return defValue;

	return value;
}

Real32 Options_GetFloat(const char* key, Real32 min, Real32 max, Real32 defValue) {
	String str;
	Real32 value;
	if (!Options_TryGetValue(key, &str))      return defValue;
	if (!Convert_TryParseReal32(&str, &value)) return defValue;

	Math_Clamp(value, min, max);
	return value;
}

UInt32 Options_GetEnum(const char* key, UInt32 defValue, const char** names, UInt32 namesCount) {
	String str;
	if (!Options_TryGetValue(key, &str)) return defValue;
	return Utils_ParseEnum(&str, defValue, names, namesCount);
}

static void Options_Remove(UInt32 i) {
	StringsBuffer_Remove(&Options_Keys, i);
	StringsBuffer_Remove(&Options_Values, i);
}

static Int32 Options_Insert(STRING_PURE String* key, STRING_PURE String* value) {
	UInt32 i = Options_Find(key);
	if (i != OPT_NOT_FOUND) Options_Remove(i);

	StringsBuffer_Add(&Options_Keys, key);
	StringsBuffer_Add(&Options_Values, value);
	return Options_Keys.Count;
}

void Options_SetBool(const char* keyRaw, bool value) {
	if (value) {
		String str = String_FromConst("True");  Options_Set(keyRaw, &str);
	} else {
		String str = String_FromConst("False"); Options_Set(keyRaw, &str);
	}
}

void Options_SetInt32(const char* keyRaw, Int32 value) {
	char numBuffer[STRING_INT_CHARS];
	String numStr = String_FromArray(numBuffer);
	String_AppendInt32(&numStr, value);
	Options_Set(keyRaw, &numStr);
}

void Options_Set(const char* keyRaw, STRING_PURE String* value) {
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
	String path = String_FromConst("options.txt");
	ReturnCode res;

	void* file; res = File_Open(&file, &path);
	if (res == ReturnCode_FileNotFound) return;
	if (res) { Chat_LogError(res, "opening", &path); return; }

	/* Remove all the unchanged options */
	UInt32 i;
	for (i = Options_Keys.Count; i > 0; i--) {
		String key = StringsBuffer_UNSAFE_Get(&Options_Keys, i - 1);
		if (Options_HasChanged(&key)) continue;
		Options_Remove(i - 1);
	}

	char lineBuffer[768];
	String line = String_FromArray(lineBuffer);
	struct Stream stream; Stream_FromFile(&stream, file);

	/* ReadLine reads single byte at a time */
	UInt8 buffer[2048]; struct Stream buffered;
	Stream_ReadonlyBuffered(&buffered, &stream, buffer, sizeof(buffer));

	for (;;) {
		res = Stream_ReadLine(&buffered, &line);
		if (res == ERR_END_OF_STREAM) break;
		if (res) { Chat_LogError(res, "reading from", &path); break; }

		if (!line.length || line.buffer[0] == '#') continue;
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

	res = stream.Close(&stream);
	if (res) { Chat_LogError(res, "closing", &path); return; }
}

void Options_Save(void) {	
	String path = String_FromConst("options.txt");
	ReturnCode res;

	void* file; res = File_Create(&file, &path);
	if (res) { Chat_LogError(res, "creating", &path); return; }

	char lineBuffer[1024];
	String line = String_FromArray(lineBuffer);
	struct Stream stream; Stream_FromFile(&stream, file);
	Int32 i;

	for (i = 0; i < Options_Keys.Count; i++) {
		String key   = StringsBuffer_UNSAFE_Get(&Options_Keys,   i);
		String value = StringsBuffer_UNSAFE_Get(&Options_Values, i);
		String_Format2(&line, "%s=%s", &key, &value);

		res = Stream_WriteLine(&stream, &line);
		if (res) { Chat_LogError(res, "writing to", &path); break; }
		line.length = 0;
	}

	StringsBuffer_Clear(&Options_Changed);
	res = stream.Close(&stream);
	if (res) { Chat_LogError(res, "closing", &path); return; }
}
