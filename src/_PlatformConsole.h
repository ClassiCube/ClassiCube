cc_bool Platform_SingleProcess = true;


/*########################################################################################################################*
*---------------------------------------------------------Memory----------------------------------------------------------*
*#########################################################################################################################*/
void Mem_Set(void*  dst, cc_uint8 value,  cc_uint32 numBytes) { memset(dst, value, numBytes); }
void Mem_Copy(void* dst, const void* src, cc_uint32 numBytes) { memcpy(dst, src,   numBytes); }

void* Mem_TryAlloc(cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	return size ? malloc(size) : NULL;
}

void* Mem_TryAllocCleared(cc_uint32 numElems, cc_uint32 elemsSize) {
	return calloc(numElems, elemsSize);
}

void* Mem_TryRealloc(void* mem, cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	return size ? realloc(mem, size) : NULL;
}

void Mem_Free(void* mem) {
	if (mem) free(mem);
}


/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
void Directory_GetCachePath(cc_string* path) { }


/*########################################################################################################################*
*-----------------------------------------------------Process/Module------------------------------------------------------*
*#########################################################################################################################*/
cc_bool Process_OpenSupported = false;

static char gameArgs[GAME_MAX_CMDARGS][STRING_SIZE];
static int gameNumArgs;
static cc_bool gameHasArgs;

cc_result Process_StartGame2(const cc_string* args, int numArgs) {
	for (int i = 0; i < numArgs; i++) 
	{
		String_CopyToRawArray(gameArgs[i], &args[i]);
	}
	
	Platform_LogConst("START GAME");
	gameHasArgs = true;
	gameNumArgs = numArgs;
	return 0;
}

static int GetGameArgs(cc_string* args) {
	int count = gameNumArgs;
	for (int i = 0; i < count; i++) 
	{
		args[i] = String_FromRawArray(gameArgs[i]);
	}
	
	// clear arguments so after game is closed, launcher is started
	gameNumArgs = 0;
	return count;
}

int Platform_GetCommandLineArgs(int argc, STRING_REF char** argv, cc_string* args) {
	if (gameHasArgs) return GetGameArgs(args);
	// Consoles *sometimes* doesn't use argv[0] for program name and so argc will be 0
	//  (e.g. when running via some emulators)
	if (!argc) return 0;
	
	argc--; argv++; // skip executable path argument

	int count = min(argc, GAME_MAX_CMDARGS);
	Platform_Log1("ARGS: %i", &count);
	
	for (int i = 0; i < count; i++) 
	{
		args[i] = String_FromReadonly(argv[i]);
		Platform_Log2("  ARG %i = %c", &i, argv[i]);
	}
	return count;
}

cc_result Platform_SetDefaultCurrentDirectory(int argc, char **argv) {
	return 0;
}

void Process_Exit(cc_result code) { exit(code); }

cc_result Process_StartOpen(const cc_string* args) {
	return ERR_NOT_SUPPORTED;
}


/*########################################################################################################################*
*--------------------------------------------------------Updater----------------------------------------------------------*
*#########################################################################################################################*/
cc_bool Updater_Supported = false;
cc_bool Updater_Clean(void) { return true; }

const struct UpdaterInfo Updater_Info = { "&eCompile latest source code to update", 0 };

cc_result Updater_Start(const char** action) {
	*action = "Starting game";
	return ERR_NOT_SUPPORTED;
}

cc_result Updater_GetBuildTime(cc_uint64* timestamp) { return ERR_NOT_SUPPORTED; }

cc_result Updater_MarkExecutable(void) { return ERR_NOT_SUPPORTED; }

cc_result Updater_SetNewBuildTime(cc_uint64 timestamp) { return ERR_NOT_SUPPORTED; }


/*########################################################################################################################*
*-------------------------------------------------------Dynamic lib-------------------------------------------------------*
*#########################################################################################################################*/
// TODO can this actually be supported somehow
const cc_string DynamicLib_Ext = String_FromConst(".so");

void* DynamicLib_Load2(const cc_string* path)      { return NULL; }
void* DynamicLib_Get2(void* lib, const char* name) { return NULL; }

cc_bool DynamicLib_DescribeError(cc_string* dst) {
	String_AppendConst(dst, "Dynamic linking unsupported");
	return true;
}


/* TODO: Copy pasted from Platform_Posix.c, bad idea */
/*########################################################################################################################*
*-------------------------------------------------------Encryption--------------------------------------------------------*
*#########################################################################################################################*/
/* Encrypts data using XTEA block cipher, with OS specific method to get machine-specific key */
static void EncipherBlock(cc_uint32* v, const cc_uint32* key, cc_string* dst) {
	cc_uint32 v0 = v[0], v1 = v[1], sum = 0, delta = 0x9E3779B9;

    for (int i = 0; i < 12; i++) 
	{
        v0  += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
        sum += delta;
        v1  += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum>>11) & 3]);
    }
    v[0] = v0; v[1] = v1;
	String_AppendAll(dst, v, 8);
}

static void DecipherBlock(cc_uint32* v, const cc_uint32* key) {
	cc_uint32 v0 = v[0], v1 = v[1], delta = 0x9E3779B9, sum = delta * 12;

    for (int i = 0; i < 12; i++) 
	{
        v1  -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum>>11) & 3]);
        sum -= delta;
        v0  -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
    }
    v[0] = v0; v[1] = v1;
}

#define ENC1 0xCC005EC0
#define ENC2 0x0DA4A0DE 
#define ENC3 0xC0DED000
#define MACHINEID_LEN 32
#define ENC_SIZE 8 /* 2 32 bit ints per block */

static cc_result GetMachineID(cc_uint32* key);

cc_result Platform_Encrypt(const void* data, int len, cc_string* dst) {
	const cc_uint8* src = (const cc_uint8*)data;
	cc_uint32 header[4], key[4] = { 0 };
	cc_result res;
	if ((res = GetMachineID(key))) return res;

	header[0] = ENC1; header[1] = ENC2;
	header[2] = ENC3; header[3] = len;
	EncipherBlock(header + 0, key, dst);
	EncipherBlock(header + 2, key, dst);

	for (; len > 0; len -= ENC_SIZE, src += ENC_SIZE) {
		header[0] = 0; header[1] = 0;
		Mem_Copy(header, src, min(len, ENC_SIZE));
		EncipherBlock(header, key, dst);
	}
	return 0;
}

cc_result Platform_Decrypt(const void* data, int len, cc_string* dst) {
	const cc_uint8* src = (const cc_uint8*)data;
	cc_uint32 header[4], key[4] = { 0 };
	cc_result res;
	int dataLen;

	/* Total size must be >= header size */
	if (len < 16) return ERR_END_OF_STREAM;
	if ((res = GetMachineID(key))) return res;

	Mem_Copy(header, src, 16);
	DecipherBlock(header + 0, key);
	DecipherBlock(header + 2, key);

	if (header[0] != ENC1 || header[1] != ENC2 || header[2] != ENC3) return ERR_INVALID_ARGUMENT;
	len -= 16; src += 16;

	if (header[3] > len) return ERR_INVALID_ARGUMENT;
	dataLen = header[3];

	for (; dataLen > 0; len -= ENC_SIZE, src += ENC_SIZE, dataLen -= ENC_SIZE) {
		header[0] = 0; header[1] = 0;
		Mem_Copy(header, src, min(len, ENC_SIZE));

		DecipherBlock(header, key);
		String_AppendAll(dst, header, min(dataLen, ENC_SIZE));
	}
	return 0;
}