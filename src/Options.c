#include "Options.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Platform.h"
#include "Stream.h"
#include "Errors.h"
#include "Utils.h"
#include "Logger.h"

struct EntryList Options;
static struct StringsBuffer changedOpts;

int Options_ChangedCount(void) { return changedOpts.count;  }

void Options_Free(void) {
	StringsBuffer_Clear(&Options.entries);
	StringsBuffer_Clear(&changedOpts);
}

cc_bool Options_HasChanged(const String* key) {
	String entry;
	int i;

	for (i = 0; i < changedOpts.count; i++) {
		entry = StringsBuffer_UNSAFE_Get(&changedOpts, i);
		if (String_CaselessEquals(&entry, key)) return true;
	}
	return false;
}

cc_bool Options_UNSAFE_Get(const char* keyRaw, String* value) {
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

cc_bool Options_GetBool(const char* key, cc_bool defValue) {
	String str;
	cc_bool value;
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

int Options_GetEnum(const char* key, int defValue, const char* const* names, int namesCount) {
	String str;
	if (!Options_UNSAFE_Get(key, &str)) return defValue;
	return Utils_ParseEnum(&str, defValue, names, namesCount);
}

void Options_SetBool(const char* keyRaw, cc_bool value) {
	static const String str_true  = String_FromConst("True");
	static const String str_false = String_FromConst("False");
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
	StringsBuffer_Add(&changedOpts, key);
}

static cc_bool Options_LoadFilter(const String* entry) {
	String key, value;
	String_UNSAFE_Separate(entry, '=', &key, &value);
	return !Options_HasChanged(&key);
}

void Options_Load(void) {
	String entry, key, value;
	int i;

	if (!Options.path) {
		EntryList_Init(&Options, "options-default.txt", '=');
		EntryList_Init(&Options, "options.txt",         '=');
	} else {
		/* Reset all the unchanged options */
		for (i = Options.entries.count - 1; i >= 0; i--) {
			entry = StringsBuffer_UNSAFE_Get(&Options.entries, i);
			String_UNSAFE_Separate(&entry, '=', &key, &value);

			if (Options_HasChanged(&key)) continue;
			StringsBuffer_Remove(&Options.entries, i);
		}

		/* Load only options which have not changed */
		EntryList_Load(&Options, "options.txt", Options_LoadFilter);
	}
}

void Options_Save(void) {
	EntryList_Save(&Options, "options.txt");
	StringsBuffer_Clear(&changedOpts);
}

void Options_SetSecure(const char* opt, const String* src, const String* key) {
	char data[2000];
	cc_uint8* enc;
	String tmp;
	int i, encLen;

	if (!src->length || !key->length) return;

	if (Platform_Encrypt(src->buffer, src->length, &enc, &encLen)) {
		/* fallback to NOT SECURE XOR. Prevents simple reading from options.txt */
		encLen = src->length;
		enc    = (cc_uint8*)Mem_Alloc(encLen, 1, "XOR encode");
	
		for (i = 0; i < encLen; i++) {
			enc[i] = (cc_uint8)(src->buffer[i] ^ key->buffer[i % key->length] ^ 0x43);
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
	cc_uint8 data[1500], c;
	int i, dataLen;
	String raw;

	Options_UNSAFE_Get(opt, &raw);
	if (!raw.length || !key->length) return;
	if (raw.length > 2000) Logger_Abort("too large to base64");

	dataLen = Convert_FromBase64(raw.buffer, raw.length, data);
	if (!Platform_Decrypt(data, dataLen, dst)) return;

	/* fallback to NOT SECURE XOR. Prevents simple reading from options.txt */
	for (i = 0; i < dataLen; i++) {
		c  = (cc_uint8)(data[i] ^ key->buffer[i % key->length] ^ 0x43);
		String_Append(dst, c);
	}
}
