#define CC_NO_UPDATER
#define CC_NO_DYNLIB
#define CC_NO_THREADING
#define CC_NO_ENCRYPTION
#define CC_NO_OPEN
#define CC_NO_CRASHHANDLER


#include "../Stream.h"
#include "../ExtMath.h"
#include "../SystemFonts.h"
#include "../Funcs.h"
#include "../Window.h"
#include "../Utils.h"
#include "../Errors.h"
#include "../PackedCol.h"

#include <errno.h>
#include <string.h>


#if defined(TARGET_SIMULATOR)
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#endif
#include "pd_api.h"

const char* Platform_AppNameSuffix = " Playdate";
const cc_result ReturnCode_SocketInProgess  = EINPROGRESS;
const cc_result ReturnCode_SocketWouldBlock = EWOULDBLOCK;
const cc_result ReturnCode_SocketDropped    = EPIPE;
cc_uint8 Platform_Flags = PLAT_FLAG_SINGLE_PROCESS | PLAT_FLAG_APP_EXIT;
cc_bool  Platform_ReadonlyFilesystem;
#undef CC_BUILD_NETWORKING // Get rid of the default Socket_ParseAddress
#include "../_PlatformBase.h"
#define CC_BUILD_NETWORKING

static bool rungame = false;
/*########################################################################################################################*
*-----------------------------------------------------Main entrypoint-----------------------------------------------------*
*#########################################################################################################################*/
#include "../main_impl.h"

static int PlaydateUpdate(void* userdata);
PlaydateAPI* pd;
#ifdef _WINDLL
__declspec(dllexport)
#endif
int eventHandler(PlaydateAPI* pd_api, PDSystemEvent event, uint32_t arg)
{
	(void)arg;
	pd_api->system->logToConsole("event: %i\n",event);
	if ( event == kEventInit )
	{
		pd = pd_api;
		SetupProgram(0, NULL);
		Launcher_Setup();
		//Game_Setup();
		pd_api->system->setUpdateCallback(PlaydateUpdate, pd);
	}
	
	return 0;
}

extern void HttpWorkerLoop(void);

static int PlaydateUpdate(void* userdata)
{
	if(!rungame)
	{
		//pd->system->logToConsole("Launcher_Tick\n");
		Launcher_Tick();
	}
	else
	{
		//pd->system->logToConsole("Game_RenderFrame\n");
		Game_RenderFrame();
	}
	//Game_RenderFrame(); 

	return 1;
}




/*########################################################################################################################*
*------------------------------------------------------Logging/Time-------------------------------------------------------*
*#########################################################################################################################*/
void Platform_Log(const char* msg, int len) {
	//int ret;
	/* Avoid "ignoring return value of 'write' declared with attribute 'warn_unused_result'" warning */
	//ret = write(STDOUT_FILENO, msg,  len);
	//ret = write(STDOUT_FILENO, "\n",   1);
	pd->system->logToConsole("%s\n",msg);
}

TimeMS DateTime_CurrentUTC(void) {
	unsigned int ms;
	return (cc_uint64)pd->system->getSecondsSinceEpoch(&ms) + UNIX_EPOCH_SECONDS;
}

void DateTime_CurrentLocal(struct cc_datetime* t) {
	struct PDDateTime pddt;
	unsigned int ms;
	pd->system->convertEpochToDateTime(pd->system->getSecondsSinceEpoch(&ms), &pddt);

	t->year   = pddt.year;
	t->month  = pddt.month;
	t->day    = pddt.day;
	t->hour   = pddt.hour;
	t->minute = pddt.minute;
	t->second = pddt.second;
}


/*########################################################################################################################*
*--------------------------------------------------------Stopwatch--------------------------------------------------------*
*#########################################################################################################################*/
#define US_PER_SEC 1000000ULL

cc_uint64 Stopwatch_Measure(void) {
	unsigned int ms;
	cc_uint64 secs = (cc_uint64)pd->system->getSecondsSinceEpoch(&ms);
	return secs*1000000+ms*1000;
}

cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;
	return end - beg;
}


/*########################################################################################################################*
*--------------------------------------------------------Threading--------------------------------------------------------*
*#########################################################################################################################*/
void Thread_Sleep(cc_uint32 milliseconds) { pd->system->delay(milliseconds); }



/*########################################################################################################################*
*-----------------------------------------------------Process/Module------------------------------------------------------*
*#########################################################################################################################*/
cc_result Process_StartGame2(const cc_string* args, int numArgs) {
	Platform_LogConst("START CLASSICUBE");
	rungame = true;
	cc_result ya = SetGameArgs(args, numArgs);
	Game_Setup();
	return ya;
}
void Process_Exit(cc_result code) {}


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
void Platform_Free(void) { }

cc_bool Platform_DescribeError(cc_result res, cc_string* dst) {
	char chars[NATIVE_STR_LEN];
	int len;

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

void Platform_Init(void) {
}


/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/

const cc_result ReturnCode_FileShareViolation = 1000000000; // not used
const cc_result ReturnCode_FileNotFound     = -1;
const cc_result ReturnCode_PathNotFound     = -1;
const cc_result ReturnCode_DirectoryExists  = -1;

void Platform_EncodePath(cc_filepath* dst, const cc_string* path) {
	int len = String_CopyToRaw(dst->buffer, sizeof(dst->buffer) - 1, path);
	dst->buffer[len] = '\0'; // Always null terminate just in case
}

void Platform_DecodePath(cc_string* dst, const cc_filepath* path) {
	String_AppendConst(dst, path->buffer);
}

void Directory_GetCachePath(cc_string* path) { }

cc_result Directory_Create2(const cc_filepath* path) {
	return pd->file->mkdir(path->buffer);
}

int File_Exists(const cc_filepath* path) {
	return true;
}

cc_result Directory_Enum(const cc_string* dirPath, void* obj, Directory_EnumCallback callback) {
	pd->system->logToConsole("Directory_Enum\n");
	return ERR_NOT_SUPPORTED;
}

cc_result File_Open(cc_file* file, const cc_filepath* path) {
	*file = pd->file->open(path->buffer,kFileReadData);
	pd->system->logToConsole("File_Open: %s\n",path->buffer);
	return (*file == NULL) ? (pd->system->logToConsole("File_Open error: %s `%s`\n",pd->file->geterr(),path->buffer),-12) : (0);
}

cc_result File_Create(cc_file* file, const cc_filepath* path) {
	*file = pd->file->open(path->buffer,kFileReadData|kFileWrite);
	return (*file == NULL) ? (pd->system->logToConsole("File_Create error: %s `%s`\n",pd->file->geterr(),path->buffer),-13) : (0);
}

cc_result File_OpenOrCreate(cc_file* file, const cc_filepath* path) {
	*file = pd->file->open(path->buffer,kFileReadData|kFileWrite);
	return (*file == NULL) ? (pd->system->logToConsole("File_OpenOrCreate error: %s `%s`\n",pd->file->geterr(),path->buffer),-14) : (0);
}

cc_result File_Read(cc_file file, void* data, cc_uint32 count, cc_uint32* bytesRead) {
	*bytesRead = pd->file->read((SDFile*)file,data,count);
	return (*bytesRead == -1) ? (pd->system->logToConsole("File_Read error: %s\n",pd->file->geterr()),-15) : (0);
}

cc_result File_Write(cc_file file, const void* data, cc_uint32 count, cc_uint32* bytesWrote) {
	*bytesWrote = pd->file->write((SDFile*)file,data,count);
	return (*bytesWrote == -1) ? (pd->system->logToConsole("File_Write error: %s\n",pd->file->geterr()),-16) : (0);
}

cc_result File_Close(cc_file file) {
	return pd->file->close((SDFile*)file);
}

cc_result File_Seek(cc_file file, int offset, int seekType) {
	pd->system->logToConsole("File_Seek %i %i\n",offset,seekType);
	return pd->file->seek((SDFile*)file, offset, seekType);
}

cc_result File_Position(cc_file file, cc_uint32* pos) {
	*pos = pd->file->tell((SDFile*)file);
	pd->system->logToConsole("File_Position %i\n",*pos);
	return 0;
}

cc_result File_Length(cc_file file, cc_uint32* len) {
	int pos = pd->file->tell((SDFile*)file);
	pd->file->seek((SDFile*)file,0,SEEK_END);
	*len = pd->file->tell((SDFile*)file);
	pd->file->seek((SDFile*)file,pos,SEEK_SET);
	//pd->system->logToConsole("File_Length %i, %i\n",ret, *len);
	return 0;
}


/*########################################################################################################################*
*---------------------------------------------------------Socket----------------------------------------------------------*
*#########################################################################################################################*/

cc_result Socket_SetNonBlocking(cc_socket s, cc_bool nonblocking) {
	return 0;
}

typedef struct
{
	int port;
	char address[128];
} PDsockaddr;

cc_result Socket_ParseAddress(const cc_string* address, int port, cc_sockaddr* addrs, int* numValidAddrs) {
	PDsockaddr* PDdata = (PDsockaddr*)addrs->data;
	String_CopyToRaw(PDdata->address,128,address);
	pd->system->logToConsole("Socket_ParseAddress %s\n",PDdata->address);
	PDdata->port = port;
	*numValidAddrs = 1;
	return 0;
}

cc_bool SockAddr_ToString(const cc_sockaddr* addr, cc_string* dst)
{
	PDsockaddr* PDdata = (PDsockaddr*)addr->data;
	*dst = String_FromRaw(PDdata->address,128);
	return true;
}

void TCPAccessCallback(bool allowed, void* userdata)
{
	pd->system->logToConsole("TCPAccessCallback: %i\n", allowed);
}

cc_result Socket_Create(cc_socket* s, cc_sockaddr* addr) {
	PDsockaddr* PDdata = (PDsockaddr*)addr->data;

	
	//int fd = socket(raw->sa_family, SOCK_STREAM, IPPROTO_TCP);
	//if (fd != -1) SetCloseOnExec(fd);
	pd->network->tcp->requestAccess(PDdata->address,PDdata->port,false,"Classicube socket 2",TCPAccessCallback,NULL);
	*s = (cc_socket)pd->network->tcp->newConnection(PDdata->address,PDdata->port,false);
	pd->system->logToConsole("Socket_Create %s %i %x\n", PDdata->address, PDdata->port, *s);
	
	return 0;
}

void Socket_Close(cc_socket s) { }

void tcpOpenCallback(TCPConnection* conn, PDNetErr err, void* ud)
{
	pd->system->logToConsole("tcpOpenCallback: %i\n", err);
}

cc_result Socket_Connect(cc_socket s, const void* addr, int addrSize) {
	PDsockaddr* PDdata = (PDsockaddr*)addr;
	
	pd->system->logToConsole("Socket_Connect %s\n", PDdata->address);
	pd->network->tcp->open((TCPConnection*)s, tcpOpenCallback, NULL);
	//s = (cc_socket)pd->network->tcp->newConnection(PDdata->address,PDdata->port,false);
	return 0;
}

cc_result Socket_Read(cc_socket s, cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	TCPConnection* conn = (TCPConnection*)s;
	pd->system->logToConsole("Socket_Read %i\n",count);
	*modified = pd->network->tcp->read(conn,data,count);
	return (((int)*modified) < 0) ? (pd->system->logToConsole("Socket_Read error: %i\n",*modified),*modified) : (0);
	//return ERR_NOT_SUPPORTED;
}

cc_result Socket_Write(cc_socket s, const cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	TCPConnection* conn = (TCPConnection*)s;
	*modified = pd->network->tcp->write(conn,data,count);
	pd->system->logToConsole("Socket_Write %i   %i\n",count, *modified);
	return (((int)*modified) < 0) ? (pd->system->logToConsole("Socket_Write error: %i\n",*modified),*modified) : (0);
}

cc_result Socket_Poll(cc_socket s, int timeoutMS, int mode, cc_bool* success) {
	pd->system->logToConsole("Socket_Poll\n");
	return ERR_NOT_SUPPORTED;
}

/*########################################################################################################################*
*-----------------------------------------------------Configuration-------------------------------------------------------*
*#########################################################################################################################*/
int Platform_GetCommandLineArgs(int argc, STRING_REF char** argv, cc_string* args) {
	int i, count;
	argc--; argv++; /* skip executable path argument */
	if (gameHasArgs) return GetGameArgs(args);

	count = min(argc, GAME_MAX_CMDARGS);
	for (i = 0; i < count; i++) 
	{
		args[i] = String_FromReadonly(argv[i]);
	}
	return count;
}

cc_result Platform_SetDefaultCurrentDirectory(void) { return 0; }

