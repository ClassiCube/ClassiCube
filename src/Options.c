#include "Options.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Platform.h"
#include "Stream.h"
#include "Chat.h"
#include "Errors.h"
#include "Utils.h"

const char* FpsLimit_Names[FPS_LIMIT_COUNT] = {
	"LimitVSync", "Limit30FPS", "Limit60FPS", "Limit120FPS", "LimitNone",
};
struct EntryList Options;
static StringsBuffer Options_Changed;

bool Options_HasAnyChanged(void) { return Options_Changed.Count > 0;  }

void Options_Free(void) {
	StringsBuffer_Clear(&Options.Entries);
	StringsBuffer_Clear(&Options_Changed);
}

bool Options_HasChanged(const String* key) {
	String entry;
	int i;

	for (i = 0; i < Options_Changed.Count; i++) {
		entry = StringsBuffer_UNSAFE_Get(&Options_Changed, i);
		if (String_CaselessEquals(&entry, key)) return true;
	}
	return false;
}

bool Options_UNSAFE_Get(const char* keyRaw, String* value) {
	int idx;
	String key = String_FromReadonly(keyRaw);

	*value = EntryList_UNSAFE_Get(&Options, &key);
	if (value->length) return true; 

	/* Fallback to without '-' (e.g. "hacks-fly" to "fly") */
	/* Needed for some very old options.txt files */
	idx = String_IndexOf(&key, '-', 0);
	if (idx == -1) return false;
	key = String_UNSAFE_SubstringAt(&key, idx + 1);

	*value = EntryList_UNSAFE_Get(&Options, &key);
	return value->length > 0;
}

void Options_Get(const char* key, String* value, const char* defValue) {
	String str;
	Options_UNSAFE_Get(key, &str);
	value->length = 0;

	if (str.length) {
		String_AppendString(value, &str);
	} else {
		String_AppendConst(value, defValue);
	}
}

int Options_GetInt(const char* key, int min, int max, int defValue) {
	String str;
	int value;
	if (!Options_UNSAFE_Get(key, &str))    return defValue;
	if (!Convert_TryParseInt(&str, &value)) return defValue;

	Math_Clamp(value, min, max);
	return value;
}

bool Options_GetBool(const char* key, bool defValue) {
	String str;
	bool value;
	if (!Options_UNSAFE_Get(key, &str))     return defValue;
	if (!Convert_TryParseBool(&str, &value)) return defValue;

	return value;
}

float Options_GetFloat(const char* key, float min, float max, float defValue) {
	String str;
	float value;
	if (!Options_UNSAFE_Get(key, &str))       return defValue;
	if (!Convert_TryParseFloat(&str, &value)) return defValue;

	Math_Clamp(value, min, max);
	return value;
}

int Options_GetEnum(const char* key, int defValue, const char** names, int namesCount) {
	String str;
	if (!Options_UNSAFE_Get(key, &str)) return defValue;
	return Utils_ParseEnum(&str, defValue, names, namesCount);
}

void Options_SetBool(const char* keyRaw, bool value) {
	static String str_true  = String_FromConst("True");
	static String str_false = String_FromConst("False");
	Options_Set(keyRaw, value ? &str_true : &str_false);
}

void Options_SetInt(const char* keyRaw, int value) {
	String str; char strBuffer[STRING_INT_CHARS];
	String_InitArray(str, strBuffer);
	String_AppendInt(&str, value);
	Options_Set(keyRaw, &str);
}

void Options_Set(const char* keyRaw, const String* value) {
	String key = String_FromReadonly(keyRaw);
	Options_SetString(&key, value);
}

void Options_SetString(const String* key, const String* value) {
	int i;
	if (!value || !value->length) {
		i = EntryList_Remove(&Options, key);
		if (i == -1) return;
	} else {
		EntryList_Set(&Options, key, value);
	}

	if (Options_HasChanged(key)) return;
	StringsBuffer_Add(&Options_Changed, key);
}

static bool Options_LoadFilter(const String* entry) {
	String key, value;
	String_UNSAFE_Separate(entry, '=', &key, &value);
	return !Options_HasChanged(&key);
}

void Options_Load(void) {
	String entry, key, value;
	int i;

	if (!Options.Filename) {
		EntryList_Init(&Options, NULL, "options.txt", '=');
	} else {
		/* Reset all the unchanged options */
		for (i = Options.Entries.Count - 1; i >= 0; i--) {
			entry = StringsBuffer_UNSAFE_Get(&Options.Entries, i);
			String_UNSAFE_Separate(&entry, '=', &key, &value);

			if (Options_HasChanged(&key)) continue;
			StringsBuffer_Remove(&Options.Entries, i);
		}

		/* Load only options which have not changed */
		EntryList_Load(&Options, Options_LoadFilter);
	}
}

void Options_Save(void) {
	EntryList_Save(&Options);
	StringsBuffer_Clear(&Options_Changed);
}
