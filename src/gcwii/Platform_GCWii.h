/*########################################################################################################################*
*-------------------------------------------------------Crash handling----------------------------------------------------*
*#########################################################################################################################*/
void CrashHandler_Install(void) { }

void Process_Abort2(cc_result result, const char* raw_msg) {
	Logger_DoAbort(result, raw_msg, NULL);
}


/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
static char root_buffer[NATIVE_STR_LEN];
static cc_string root_path = String_FromArray(root_buffer);

static bool fat_available; 
// trying to call mkdir etc with no FAT device loaded seems to be broken (dolphin crashes due to trying to execute invalid instruction)
//   https://github.com/Patater/newlib/blob/8a9e3aaad59732842b08ad5fc19e0acf550a418a/libgloss/libsysbase/mkdir.c and
//   https://github.com/Patater/newlib/blob/8a9e3aaad59732842b08ad5fc19e0acf550a418a/newlib/libc/include/sys/iosupport.h
// FindDevice() returns -1 when no matching device, however the code still unconditionally does "if (devoptab_list[dev]->mkdir_r) {"
// - so will either attempt to access or execute invalid memory

void Platform_EncodePath(cc_filepath* dst, const cc_string* path) {
	char* str = dst->buffer;
	Mem_Copy(str, root_path.buffer, root_path.length);
	str   += root_path.length;
	*str++ = '/';
	String_EncodeUtf8(str, path);
}

void Platform_DecodePath(cc_string* dst, const cc_filepath* path) {
	const char* str = path->buffer;
	String_AppendUtf8(dst, str, String_Length(str));
}

void Directory_GetCachePath(cc_string* path) { }

cc_result Directory_Create2(const cc_filepath* path) {
	if (!fat_available) return ERR_NON_WRITABLE_FS;

	return mkdir(path->buffer, 0) == -1 ? errno : 0;
}

int File_Exists(const cc_filepath* path) {
	if (!fat_available) return false;
	
	struct stat sb;
	return stat(path->buffer, &sb) == 0 && S_ISREG(sb.st_mode);
}

cc_result Directory_Enum(const cc_string* dirPath, void* obj, Directory_EnumCallback callback) {
	if (!fat_available) return ENOSYS;

	cc_string path; char pathBuffer[FILENAME_SIZE];
	cc_filepath str;
	struct dirent* entry;
	int res;

	Platform_EncodePath(&str, dirPath);
	DIR* dirPtr = opendir(str.buffer);
	if (!dirPtr) return errno;

	String_InitArray(path, pathBuffer);

	do {
		// POSIX docs: "When the end of the directory is encountered, a null pointer is returned and errno is not changed."
		// errno is sometimes leftover from previous calls, so always reset it before readdir gets called
		errno = 0;
		entry = readdir(dirPtr);
		if (!entry) continue;

		path.length = 0;
		String_Format1(&path, "%s/", dirPath);

		// ignore . and .. entry
		char* src = entry->d_name;
		if (src[0] == '.' && src[1] == '\0') continue;
		if (src[0] == '.' && src[1] == '.' && src[2] == '\0') continue;

		int len = String_Length(src);
		String_AppendUtf8(&path, src, len);
		int is_dir = entry->d_type == DT_DIR;
		// TODO: fallback to stat when this fails

		callback(&path, obj, is_dir);
	} while (entry || errno == EOVERFLOW);

	res = errno; // return code from readdir
	closedir(dirPtr);
	return res;
}

static cc_result File_Do(cc_file* file, const char* path, int mode) {
	*file = open(path, mode, 0);
	return *file == -1 ? errno : 0;
}

cc_result File_Open(cc_file* file, const cc_filepath* path) {
	if (!fat_available) return ReturnCode_FileNotFound;
	return File_Do(file, path->buffer, O_RDONLY);
}

cc_result File_Create(cc_file* file, const cc_filepath* path) {
	if (!fat_available) return ERR_NON_WRITABLE_FS;
	return File_Do(file, path->buffer, O_RDWR | O_CREAT | O_TRUNC);
}

cc_result File_OpenOrCreate(cc_file* file, const cc_filepath* path) {
	if (!fat_available) return ERR_NON_WRITABLE_FS;
	return File_Do(file, path->buffer, O_RDWR | O_CREAT);
}

cc_result File_Read(cc_file file, void* data, cc_uint32 count, cc_uint32* bytesRead) {
	*bytesRead = read(file, data, count);
	return *bytesRead == -1 ? errno : 0;
}

cc_result File_Write(cc_file file, const void* data, cc_uint32 count, cc_uint32* bytesWrote) {
	*bytesWrote = write(file, data, count);
	return *bytesWrote == -1 ? errno : 0;
}

cc_result File_Close(cc_file file) {
	return close(file) == -1 ? errno : 0;
}

cc_result File_Seek(cc_file file, int offset, int seekType) {
	static cc_uint8 modes[3] = { SEEK_SET, SEEK_CUR, SEEK_END };
	return lseek(file, offset, modes[seekType]) == -1 ? errno : 0;
}

cc_result File_Position(cc_file file, cc_uint32* pos) {
	*pos = lseek(file, 0, SEEK_CUR);
	return *pos == -1 ? errno : 0;
}

cc_result File_Length(cc_file file, cc_uint32* len) {
	struct stat st;
	if (fstat(file, &st) == -1) { *len = -1; return errno; }
	*len = st.st_size; return 0;
}


/*########################################################################################################################*
*--------------------------------------------------------Threading--------------------------------------------------------*
*#########################################################################################################################*/
void Thread_Sleep(cc_uint32 milliseconds) { usleep(milliseconds * 1000); }

static void* ExecThread(void* param) {
	((Thread_StartFunc)param)(); 
	return NULL;
}

void Thread_Run(void** handle, Thread_StartFunc func, int stackSize, const char* name) {
	lwp_t* thread = (lwp_t*)Mem_Alloc(1, sizeof(lwp_t), "thread");
	*handle = thread;
	
	int res = LWP_CreateThread(thread, ExecThread, (void*)func, NULL, stackSize, 64);
	if (res) Process_Abort2(res, "Creating thread");
}

void Thread_Detach(void* handle) {
	// TODO: Leaks return value of thread ???
	lwp_t* ptr = (lwp_t*)handle;
#ifndef HW_RVL
	LWP_DetachThread(*ptr);
#endif
	Mem_Free(ptr);
}

void Thread_Join(void* handle) {
	lwp_t* ptr = (lwp_t*)handle;
	int res = LWP_JoinThread(*ptr, NULL);
	if (res) Process_Abort2(res, "Joining thread");
	Mem_Free(ptr);
}


/*########################################################################################################################*
*---------------------------------------------------------Socket----------------------------------------------------------*
*#########################################################################################################################*/
static cc_bool net_supported = true;

cc_bool SockAddr_ToString(const cc_sockaddr* addr, cc_string* dst) {
	struct sockaddr_in* addr4 = (struct sockaddr_in*)addr->data;

	if (addr4->sin_family == AF_INET) 
		return IPv4_ToString(&addr4->sin_addr, &addr4->sin_port, dst);
	return false;
}

static cc_bool ParseIPv4(const cc_string* ip, int port, cc_sockaddr* dst) {
	struct sockaddr_in* addr4 = (struct sockaddr_in*)dst->data;
	cc_uint32 ip_addr = 0;
	if (!ParseIPv4Address(ip, &ip_addr)) return false;

	addr4->sin_addr.s_addr = ip_addr;
	addr4->sin_family      = AF_INET;
	addr4->sin_port        = SockAddr_EncodePort(port);
		
	dst->size = sizeof(*addr4);
	return true;
}

static cc_bool ParseIPv6(const char* ip, int port, cc_sockaddr* dst) {
	return false;
}

cc_result Socket_Create(cc_socket* s, cc_sockaddr* addr, cc_bool nonblocking) {
	struct sockaddr* raw = (struct sockaddr*)addr->data;
	if (!net_supported) { *s = -1; return ERR_NO_NETWORKING; }

	*s = net_socket(raw->sa_family, SOCK_STREAM, IPPROTO_IP);
	if (*s < 0) return *s;

	if (nonblocking) {
		int blocking_raw = 1; /* non-blocking mode */
		net_ioctl(*s, FIONBIO, &blocking_raw);
	}
	return 0;
}

cc_result Socket_Connect(cc_socket s, cc_sockaddr* addr) {
	struct sockaddr* raw = (struct sockaddr*)addr->data;
	
	int res = net_connect(s, raw, addr->size);
	return res < 0 ? res : 0;
}

cc_result Socket_Read(cc_socket s, cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int res = net_recv(s, data, count, 0);
	if (res < 0) { *modified = 0; return res; }
	
	*modified = res; return 0;
}

cc_result Socket_Write(cc_socket s, const cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int res = net_send(s, data, count, 0);
	if (res < 0) { *modified = 0; return res; }
	
	*modified = res; return 0;
}

void Socket_Close(cc_socket s) {
	net_shutdown(s, 2); // SHUT_RDWR = 2
	net_close(s);
}

static cc_result Socket_Poll(cc_socket s, int mode, cc_bool* success);

cc_result Socket_CheckReadable(cc_socket s, cc_bool* readable) {
	return Socket_Poll(s, SOCKET_POLL_READ, readable);
}

cc_result Socket_CheckWritable(cc_socket s, cc_bool* writable) {
	return Socket_Poll(s, SOCKET_POLL_WRITE, writable);
}

static void InitSockets(void);


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
static void AppendDevice(cc_string* path, char* cwd) {
	// try to find device FAT mounted on, otherwise default to SD card
	if (!cwd) { String_AppendConst(path, "sd"); return;	}
	
	Platform_Log1("CWD: %c", cwd);
	cc_string cwd_ = String_FromReadonly(cwd);
	int deviceEnd  = String_IndexOf(&cwd_, ':');
		
	if (deviceEnd >= 0) {
		// e.g. "card0:/" becomes "card0"
		String_AppendAll(path, cwd, deviceEnd);
	} else {
		String_AppendConst(path, "sd");
	}
}

static void FindRootDirectory(void) {
	char cwdBuffer[NATIVE_STR_LEN] = { 0 };
	char* cwd = getcwd(cwdBuffer, NATIVE_STR_LEN);
	
	root_path.length = 0;
	AppendDevice(&root_path, cwd);
	String_AppendConst(&root_path, ":/ClassiCube");
}

static void CreateRootDirectory(void) {
	if (!fat_available) return;
	root_buffer[root_path.length] = '\0';
	
	// Directory_Create2(&String_Empty); just returns error 20
	int res = mkdir(root_buffer, 0);
	int err = res == -1 ? errno : 0;
	Platform_Log1("Created root directory: %i", &err);
}

void Platform_Init(void) {
	fat_available = fatInitDefault();
	Platform_ReadonlyFilesystem = !fat_available;

	FindRootDirectory();
	CreateRootDirectory();
	
	InitSockets();
}
void Platform_Free(void) { }

cc_bool Platform_DescribeError(cc_result res, cc_string* dst) {
	char chars[NATIVE_STR_LEN];
	int len;

	if (res == ERR_NON_WRITABLE_FS) {
		String_AppendConst(dst, "No supported SD card detected\n    Will not be able to save any files");
		return true;
	}

	/* For unrecognised error codes, strerror_r might return messages */
	/*  such as 'No error information', which is not very useful */
	/* (could check errno here but quicker just to skip entirely) */
	if (res >= 1000) return false;

	len = strerror_r(res, chars, NATIVE_STR_LEN);
	if (len == -1) return false;

	len = String_CalcLen(chars, NATIVE_STR_LEN);
	String_AppendUtf8(dst, chars, len);
	return true;
}

cc_bool Process_OpenSupported = false;
cc_result Process_StartOpen(const cc_string* args) {
	return ERR_NOT_SUPPORTED;
}

void Process_Exit(cc_result code) { exit(code); }

cc_result Process_StartGame2(const cc_string* args, int numArgs) {
	Platform_LogConst("START CLASSICUBE");
	return SetGameArgs(args, numArgs);
}

cc_result Platform_SetDefaultCurrentDirectory(int argc, char **argv) {
	return 0;
}

void CPU_FlushDataCache(void* start, int length) {
	DCFlushRange(start, length);
}

void CPU_InvalidateDataCache(void* start, int length) {
	DCInvalidateRange(start, length);
}

