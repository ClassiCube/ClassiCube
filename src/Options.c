#include "Options.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Platform.h"
#include "Stream.h"
#include "Chat.h"
#include "Errors.h"
#include "Utils.h"
#include "Logger.h"

const char* FpsLimit_Names[FPS_LIMIT_COUNT] = {
	"LimitVSync", "Limit30FPS", "Limit60FPS", "Limit120FPS", "Limit144FPS", "LimitNone",
};
struct EntryList Options;
static StringsBuffer Options_Changed;

int Options_ChangedCount(void) { return Options_Changed.Count;  }

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
	idx = String_IndexOf(&key, '-');
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
	if (!Options_UNSAFE_Get(key, &str))  return defValue;
	if (!Convert_ParseInt(&str, &value)) return defValue;

	Math_Clamp(value, min, max);
	return value;
}

bool Options_GetBool(const char* key, bool defValue) {
	String str;
	bool value;
	if (!Options_UNSAFE_Get(key, &str))   return defValue;
	if (!Convert_ParseBool(&str, &value)) return defValue;

	return value;
}

float Options_GetFloat(const char* key, float min, float max, float defValue) {
	String str;
	float value;
	if (!Options_UNSAFE_Get(key, &str))    return defValue;
	if (!Convert_ParseFloat(&str, &value)) return defValue;

	Math_Clamp(value, min, max);
	return value;
}

int Options_GetEnum(const char* key, int defValue, const char** names, int namesCount) {
	String str;
	if (!Options_UNSAFE_Get(key, &str)) return defValue;
	return Utils_ParseEnum(&str, defValue, names, namesCount);
}

void Options_SetBool(const char* keyRaw, bool value) {
	const static String str_true  = String_FromConst("True");
	const static String str_false = String_FromConst("False");
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

#ifdef CC_BUILD_WEB
	Options_Save();
#endif

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

void Options_SetSecure(const char* opt, const String* src, const String* key) {
	char data[2000];
	uint8_t* enc;
	String tmp;
	int i, encLen;

	if (!src->length || !key->length) return;

	if (Platform_Encrypt(src->buffer, src->length, &enc, &encLen)) {
		/* fallback to NOT SECURE XOR. Prevents simple reading from options.txt */
		encLen = src->length;
		enc    = Mem_Alloc(encLen, 1, "XOR encode");
	
		for (i = 0; i < encLen; i++) {
			enc[i] = (uint8_t)(src->buffer[i] ^ key->buffer[i % key->length] ^ 0x43);
		}
	}

	if (encLen > 1500) Logger_Abort("too large to base64");
	tmp.buffer   = data;
	tmp.length   = Convert_ToBase64(enc, encLen, data);
	tmp.capacity = tmp.length;

	Options_Set(opt, &tmp);
	Mem_Free(enc);
}

void Options_GetSecure(const char* opt, String* dst, const String* key) {
	uint8_t data[1500];
	uint8_t* dec;
	String raw;
	int i, decLen, dataLen;

	Options_UNSAFE_Get(opt, &raw);
	if (!raw.length || !key->length) return;
	if (raw.length > 2000) Logger_Abort("too large to base64");
	dataLen = Convert_FromBase64(raw.buffer, raw.length, data);

	if (Platform_Decrypt(data, dataLen, &dec, &decLen)) {
		/* fallback to NOT SECURE XOR. Prevents simple reading from options.txt */
		decLen = dataLen;
		dec    = Mem_Alloc(decLen, 1, "XOR decode");

		for (i = 0; i < decLen; i++) {
			dec[i] = (uint8_t)(data[i] ^ key->buffer[i % key->length] ^ 0x43);
		}
	}

	for (i = 0; i < decLen; i++) {
		String_Append(dst, dec[i]);
	}
	Mem_Free(dec);
}
