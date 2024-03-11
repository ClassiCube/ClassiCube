#include "Options.h"
#include "String.h"
#include "ExtMath.h"
#include "Platform.h"
#include "Stream.h"
#include "Errors.h"
#include "Utils.h"
#include "Logger.h"
#include "PackedCol.h"

struct StringsBuffer Options;
static struct StringsBuffer changedOpts;
cc_result Options_LoadResult;
static cc_bool savingPaused;
#if defined CC_BUILD_WEB || defined CC_BUILD_MOBILE || defined CC_BUILD_CONSOLE
	#define OPTIONS_SAVE_IMMEDIATELY
#endif

void Options_Free(void) {
	StringsBuffer_Clear(&Options);
	StringsBuffer_Clear(&changedOpts);
}

static cc_bool HasChanged(const cc_string* key) {
	cc_string entry;
	int i;

	for (i = 0; i < changedOpts.count; i++) {
		entry = StringsBuffer_UNSAFE_Get(&changedOpts, i);
		if (String_CaselessEquals(&entry, key)) return true;
	}
	return false;
}

static cc_bool Options_LoadFilter(const cc_string* entry) {
	cc_string key, value;
	String_UNSAFE_Separate(entry, '=', &key, &value);
	return !HasChanged(&key);
}

void Options_Load(void) {
	/* Increase from max 512 to 2048 per entry */
	StringsBuffer_SetLengthBits(&Options, 11);
	Options_LoadResult = EntryList_Load(&Options, "options-default.txt", '=', NULL);
	Options_LoadResult = EntryList_Load(&Options, "options.txt",         '=', NULL);
}

void Options_Reload(void) {
	cc_string entry, key, value;
	int i;

	/* Reset all the unchanged options */
	for (i = Options.count - 1; i >= 0; i--) {
		entry = StringsBuffer_UNSAFE_Get(&Options, i);
		String_UNSAFE_Separate(&entry, '=', &key, &value);

		if (HasChanged(&key)) continue;
		StringsBuffer_Remove(&Options, i);
	}
	/* Load only options which have not changed */
	Options_LoadResult = EntryList_Load(&Options, "options.txt", '=', Options_LoadFilter);
}

static void SaveOptions(void) {
	EntryList_Save(&Options, "options.txt");
	StringsBuffer_Clear(&changedOpts);
}

void Options_SaveIfChanged(void) {
	if (!changedOpts.count) return;
	
	Options_Reload();
	SaveOptions();
}

void Options_PauseSaving(void) { savingPaused = true; }

void Options_ResumeSaving(void) { 
	savingPaused = false;
#if defined OPTIONS_SAVE_IMMEDIATELY
	SaveOptions();
#endif
}


cc_bool Options_UNSAFE_Get(const char* keyRaw, cc_string* value) {
	int idx;
	cc_string key = String_FromReadonly(keyRaw);

	*value = EntryList_UNSAFE_Get(&Options, &key, '=');
	if (value->length) return true; 

	/* Fallback to without '-' (e.g. "hacks-fly" to "fly") */
	/* Needed for some very old options.txt files */
	idx = String_IndexOf(&key, '-');
	if (idx == -1) return false;
	key = String_UNSAFE_SubstringAt(&key, idx + 1);

	*value = EntryList_UNSAFE_Get(&Options, &key, '=');
	return value->length > 0;
}

void Options_Get(const char* key, cc_string* value, const char* defValue) {
	cc_string str;
	Options_UNSAFE_Get(key, &str);
	value->length = 0;

	if (str.length) {
		String_AppendString(value, &str);
	} else {
		String_AppendConst(value, defValue);
	}
}

int Options_GetInt(const char* key, int min, int max, int defValue) {
	cc_string str;
	int value;
	if (!Options_UNSAFE_Get(key, &str))  return defValue;
	if (!Convert_ParseInt(&str, &value)) return defValue;

	Math_Clamp(value, min, max);
	return value;
}

cc_bool Options_GetBool(const char* key, cc_bool defValue) {
	cc_string str;
	cc_bool value;
	if (!Options_UNSAFE_Get(key, &str))   return defValue;
	if (!Convert_ParseBool(&str, &value)) return defValue;

	return value;
}

float Options_GetFloat(const char* key, float min, float max, float defValue) {
	cc_string str;
	float value;
	if (!Options_UNSAFE_Get(key, &str))    return defValue;
	if (!Convert_ParseFloat(&str, &value)) return defValue;

	Math_Clamp(value, min, max);
	return value;
}

int Options_GetEnum(const char* key, int defValue, const char* const* names, int namesCount) {
	cc_string str;
	if (!Options_UNSAFE_Get(key, &str)) return defValue;
	return Utils_ParseEnum(&str, defValue, names, namesCount);
}

cc_bool Options_GetColor(const char* key, cc_uint8* rgb) {
	cc_string value, parts[3];
	if (!Options_UNSAFE_Get(key, &value))   return false;
	if (PackedCol_TryParseHex(&value, rgb)) return true;

	/* Try parsing as R,G,B instead */
	return String_UNSAFE_Split(&value, ',', parts, 3)
		&& Convert_ParseUInt8(&parts[0], &rgb[0])
		&& Convert_ParseUInt8(&parts[1], &rgb[1])
		&& Convert_ParseUInt8(&parts[2], &rgb[2]);
}


void Options_SetBool(const char* keyRaw, cc_bool value) {
	static const cc_string str_true  = String_FromConst("True");
	static const cc_string str_false = String_FromConst("False");
	Options_Set(keyRaw, value ? &str_true : &str_false);
}

void Options_SetInt(const char* keyRaw, int value) {
	cc_string str; char strBuffer[STRING_INT_CHARS];
	String_InitArray(str, strBuffer);
	String_AppendInt(&str, value);
	Options_Set(keyRaw, &str);
}

void Options_Set(const char* keyRaw, const cc_string* value) {
	cc_string key = String_FromReadonly(keyRaw);
	Options_SetString(&key, value);
}

void Options_SetString(const cc_string* key, const cc_string* value) {
	if (!value || !value->length) {
		if (!EntryList_Remove(&Options, key, '=')) return;
	} else {
		EntryList_Set(&Options, key, value, '=');
	}

#if defined OPTIONS_SAVE_IMMEDIATELY
	if (!savingPaused) SaveOptions();
#endif

	if (HasChanged(key)) return;
	StringsBuffer_Add(&changedOpts, key);
}

void Options_SetSecure(const char* opt, const cc_string* src) {
	char data[2000], encData[1500+1];
	cc_string tmp, enc;
	cc_result res;
	if (!src->length) return;

	String_InitArray(enc, encData);
	res = Platform_Encrypt(src->buffer, src->length, &enc);
	if (res) { Platform_Log2("Error %h encrypting option %c", &res, opt); return; }

	/* base64 encode the data, as user might edit options.txt with a text editor */
	if (enc.length > 1500) Logger_Abort("too large to base64");
	tmp.buffer   = data;
	tmp.length   = Convert_ToBase64(enc.buffer, enc.length, data);
	tmp.capacity = tmp.length;
	Options_Set(opt, &tmp);
}

void Options_GetSecure(const char* opt, cc_string* dst) {
	cc_uint8 data[1500];
	int dataLen;
	cc_string raw;
	cc_result res;

	Options_UNSAFE_Get(opt, &raw);
	if (!raw.length) return;
	if (raw.length > 2000) Logger_Abort("too large to base64");

	dataLen = Convert_FromBase64(raw.buffer, raw.length, data);
	res = Platform_Decrypt(data, dataLen, dst);
	if (res) Platform_Log2("Error %h decrypting option %c", &res, opt);
}
