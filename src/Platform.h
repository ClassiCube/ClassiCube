#ifndef CC_PLATFORM_H
#define CC_PLATFORM_H
#include "String.h"
/* Abstracts platform specific memory management, I/O, etc.
   Copyright 2014-2020 ClassiCube | Licensed under BSD-3
*/
struct DateTime;

enum Socket_PollMode { SOCKET_POLL_READ, SOCKET_POLL_WRITE };
#ifdef CC_BUILD_WIN
typedef cc_uintptr cc_socket;
typedef void* cc_file;
#define _NL "\r\n"
#define NATIVE_STR_LEN 300
#else
typedef int cc_socket;
typedef int cc_file;
#define _NL "\n"
#define NATIVE_STR_LEN 600
#endif
#define UPDATE_FILE "ClassiCube.update"

/* Origin points for when seeking in a file. */
enum File_SeekFrom { FILE_SEEKFROM_BEGIN, FILE_SEEKFROM_CURRENT, FILE_SEEKFROM_END };
/* Number of milliseconds since 01/01/0001 to start of unix time. */
#define UNIX_EPOCH 62135596800000ULL

extern const cc_result ReturnCode_FileShareViolation;
extern const cc_result ReturnCode_FileNotFound;
extern const cc_result ReturnCode_SocketInProgess;
extern const cc_result ReturnCode_SocketWouldBlock;

/* Encodes a string in platform specific format. (e.g. unicode on windows, UTF8 on linux) */
/* NOTE: Only useful for platform specific function calls - do NOT try to interpret the data. */
/* Returns the number of bytes written, excluding trailing NULL terminator. */
CC_API int Platform_ConvertString(void* data, const String* src);

/* Initialises the platform specific state. */
void Platform_Init(void);
/* Frees the platform specific state. */
void Platform_Free(void);
/* Sets the appropriate default current/working directory. */
cc_result Platform_SetDefaultCurrentDirectory(int argc, char **argv);
/* Gets the command line arguments passed to the program. */
int Platform_GetCommandLineArgs(int argc, STRING_REF char** argv, String* args);

/* Encrypts data in a platform-specific manner. (may not be supported) */
/* NOTE: Should only be implemented when platform natively supports it. */
cc_result Platform_Encrypt(const void* data, int len, cc_uint8** enc, int* encLen);
/* Decrypts data in a platform-specific manner. (may not be supported) */
/* NOTE: Should only be implemented when platform natively supports it. */
cc_result Platform_Decrypt(const void* data, int len, String* dst);
/* Outputs more detailed information about errors with operating system functions. */
/* NOTE: This is for general functions like file I/O. If a more specific 
describe exists (e.g. Http_DescribeError), that should be preferred. */
cc_bool Platform_DescribeError(cc_result res, String* dst);

/* Starts the game with the given arguments. */
CC_API cc_result Process_StartGame(const String* args);
/* Terminates the process with the given return code. */
CC_API void Process_Exit(cc_result code);
/* Starts the platform-specific program to open the given url or filename. */
/* For example, provide a http:// url to open a website in the user's web browser. */
CC_API void Process_StartOpen(const String* args);

extern const char* const Updater_D3D9;
extern const char* const Updater_OGL;
/* Attempts to clean up any leftover files from an update */
cc_bool Updater_Clean(void);
/* Starts the platform-specific method to update then start the game using the UPDATE_FILE file. */
cc_result Updater_Start(void);
/* Returns the last time the application was modified, as a unix timestamp. */
cc_result Updater_GetBuildTime(cc_uint64* timestamp);
/* Marks the UPDATE_FILE file as being executable. (Needed for some platforms) */
cc_result Updater_MarkExecutable(void);
/* Sets the last time UPDATE_FILE file was modified, as a unix timestamp. */
cc_result Updater_SetNewBuildTime(cc_uint64 timestamp);

/* The default file extension used for dynamic libraries on this platform. */
extern const String DynamicLib_Ext;
CC_API cc_result DynamicLib_Load(const String* path, void** lib); /* OBSOLETE */
CC_API cc_result DynamicLib_Get(void* lib, const char* name, void** symbol); /* OBSOLETE */

/* Attempts to load a native dynamic library from the given path. */
CC_API void* DynamicLib_Load2(const String* path);
/* Attempts to get the address of the symbol in the given dynamic library. */
/* NOTE: Do NOT use this to load OpenGL functions, use GLContext_GetAddress. */
CC_API void* DynamicLib_Get2(void* lib, const char* name);
/* Outputs more detailed information about errors with the DynamicLib functions. */
/* NOTE: You MUST call this immediately after DynamicLib_Load2/DynamicLib_Get2 returns NULL. */
CC_API cc_bool DynamicLib_DescribeError(String* dst);

/* Allocates a block of memory, with undetermined contents. Returns NULL on allocation failure. */
CC_API void* Mem_TryAlloc(cc_uint32 numElems, cc_uint32 elemsSize);
/* Allocates a block of memory, with contents of all 0. Returns NULL on allocation failure. */
CC_API void* Mem_TryAllocCleared(cc_uint32 numElems, cc_uint32 elemsSize);
/* Reallocates a block of memory, with undetermined contents. Returns NULL on reallocation failure. */
CC_API void* Mem_TryRealloc(void* mem, cc_uint32 numElems, cc_uint32 elemsSize);

/* Allocates a block of memory, with undetermined contents. Exits process on allocation failure. */
CC_API void* Mem_Alloc(cc_uint32 numElems, cc_uint32 elemsSize, const char* place);
/* Allocates a block of memory, with contents of all 0. Exits process on allocation failure. */
CC_API void* Mem_AllocCleared(cc_uint32 numElems, cc_uint32 elemsSize, const char* place);
/* Reallocates a block of memory, with undetermined contents. Exits process on reallocation failure. */
CC_API void* Mem_Realloc(void* mem, cc_uint32 numElems, cc_uint32 elemsSize, const char* place);
/* Frees an allocated a block of memory. Does nothing when passed NULL. */
CC_API void  Mem_Free(void* mem);
/* Sets the contents of a block of memory to the given value. */
void Mem_Set(void* dst, cc_uint8 value, cc_uint32 numBytes);
/* Copies a block of memory to another block of memory. */
/* NOTE: These blocks MUST NOT overlap. */
void Mem_Copy(void* dst, const void* src, cc_uint32 numBytes);

/* Logs a debug message to console. */
void Platform_Log(const String* message);
void Platform_LogConst(const char* message);
void Platform_Log1(const char* format, const void* a1);
void Platform_Log2(const char* format, const void* a1, const void* a2);
void Platform_Log3(const char* format, const void* a1, const void* a2, const void* a3);
void Platform_Log4(const char* format, const void* a1, const void* a2, const void* a3, const void* a4);

/* Returns the current UTC time, as number of milliseconds since 1/1/0001 */
CC_API TimeMS DateTime_CurrentUTC_MS(void);
/* Returns the current local Time. */
CC_API void DateTime_CurrentLocal(struct DateTime* t);
/* Takes a platform-specific stopwatch measurement. */
/* NOTE: The value returned is platform-specific - do NOT try to interpret the value. */
CC_API cc_uint64 Stopwatch_Measure(void);
/* Returns total elapsed microseconds between two stopwatch measurements. */
CC_API cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end);
/* Returns total elapsed milliseconds between two stopwatch measurements. */
CC_API cc_uint64 Stopwatch_ElapsedMilliseconds(cc_uint64 beg, cc_uint64 end);

/* Returns non-zero if the given directory exists. */
CC_API int Directory_Exists(const String* path);
/* Attempts to create a new directory. */
CC_API cc_result Directory_Create(const String* path);
/* Callback function invoked for each file found. */
typedef void Directory_EnumCallback(const String* filename, void* obj);
/* Invokes a callback function on all filenames in the given directory (and its sub-directories) */
CC_API cc_result Directory_Enum(const String* path, void* obj, Directory_EnumCallback callback);
/* Returns non-zero if the given file exists. */
CC_API int File_Exists(const String* path);

/* Attempts to create a new (or overwrite) file for writing. */
/* NOTE: If the file already exists, its contents are discarded. */
cc_result File_Create(cc_file* file, const String* path);
/* Attempts to open an existing file for reading. */
cc_result File_Open(cc_file* file, const String* path);
/* Attempts to open an existing or create a new file for reading and writing. */
cc_result File_OpenOrCreate(cc_file* file, const String* path);
/* Attempts to read data from the file. */
cc_result File_Read(cc_file file, cc_uint8* data, cc_uint32 count, cc_uint32* bytesRead);
/* Attempts to write data to the file. */
cc_result File_Write(cc_file file, const cc_uint8* data, cc_uint32 count, cc_uint32* bytesWrote);
/* Attempts to close the given file. */
cc_result File_Close(cc_file file);
/* Attempts to seek to a position in the given file. */
cc_result File_Seek(cc_file file, int offset, int seekType);
/* Attempts to get the current position in the given file. */
cc_result File_Position(cc_file file, cc_uint32* pos);
/* Attempts to retrieve the length of the given file. */
cc_result File_Length(cc_file file, cc_uint32* len);

/* Blocks the current thread for the given number of milliseconds. */
CC_API void Thread_Sleep(cc_uint32 milliseconds);
typedef void (*Thread_StartFunc)(void);
/* Starts a new thread, optionally immediately detaching it. (See Thread_Detach) */
CC_API void* Thread_Start(Thread_StartFunc func, cc_bool detach);
/* Frees the platform specific persistent data associated with the thread. */
/* NOTE: You must either detach or join threads, as this data otherwise leaks. */
CC_API void Thread_Detach(void* handle);
/* Blocks the current thread, until the given thread has finished. */
/* NOTE: Once a thread has been detached, you can no longer use this method. */
CC_API void Thread_Join(void* handle);

/* Allocates a new mutex. (used to synchronise access to a shared resource) */
CC_API void* Mutex_Create(void);
/* Frees an allocated mutex. */
CC_API void  Mutex_Free(void* handle);
/* Locks the given mutex, blocking other threads from entering. */
CC_API void  Mutex_Lock(void* handle);
/* Unlocks the given mutex, allowing other threads to enter. */
CC_API void  Mutex_Unlock(void* handle);

/* Allocates a new waitable. (used to conditionally wake-up a blocked thread) */
CC_API void* Waitable_Create(void);
/* Frees an allocated waitable. */
CC_API void  Waitable_Free(void* handle);
/* Signals a waitable, waking up blocked threads. */
CC_API void  Waitable_Signal(void* handle);
/* Blocks the calling thread until the waitable gets signalled. */
CC_API void  Waitable_Wait(void* handle);
/* Blocks the calling thread until the waitable gets signalled, or milliseconds delay passes. */
CC_API void  Waitable_WaitFor(void* handle, cc_uint32 milliseconds);

void Platform_LoadSysFonts(void);

/* Allocates a new socket. */
CC_API cc_result Socket_Create(cc_socket* socket);
/* Returns how much data is available to be read from the given socket. */
CC_API cc_result Socket_Available(cc_socket socket, cc_uint32* available);
/* Sets whether operations on the given socket block the calling thread. */
/* e.g. if blocking is on, calling Connect() blocks until a response is received. */
CC_API cc_result Socket_SetBlocking(cc_socket socket, cc_bool blocking);
/* Returns (and resets) the last error generated by the given socket. */
CC_API cc_result Socket_GetError(cc_socket socket, cc_result* result);

/* Attempts to open a connection to the given IP address:port. */
CC_API cc_result Socket_Connect(cc_socket socket, const String* ip, int port);
/* Attempts to read data from the given socket. */
CC_API cc_result Socket_Read(cc_socket socket, cc_uint8* data, cc_uint32 count, cc_uint32* modified);
/* Attempts to write data to the given socket. */
CC_API cc_result Socket_Write(cc_socket socket, const cc_uint8* data, cc_uint32 count, cc_uint32* modified);
/* Attempts to close the given socket. */
CC_API cc_result Socket_Close(cc_socket socket);
/* Attempts to poll the given socket for readability or writability. */
/* NOTE: A closed socket is still considered readable. */
/* NOTE: A socket is considered writable once it has finished connecting. */
CC_API cc_result Socket_Poll(cc_socket socket, int mode, cc_bool* success);

#ifdef CC_BUILD_ANDROID
#include <jni.h>
extern jclass  App_Class;
extern jobject App_Instance;
extern JavaVM* VM_Ptr;

#define JavaGetCurrentEnv(env) (*VM_Ptr)->AttachCurrentThread(VM_Ptr, &env, NULL);
#define JavaMakeConst(env, str) (*env)->NewStringUTF(env, str);
#define JavaRegisterNatives(env, methods) (*env)->RegisterNatives(env, App_Class, methods, Array_Elems(methods));

/* Allocates a string from the given java string. */
/* NOTE: Don't forget to call env->ReleaseStringUTFChars. Only works with ASCII strings. */
String JavaGetString(JNIEnv* env, jstring str);
/* Allocates a java string from the given string. */
jobject JavaMakeString(JNIEnv* env, const String* str);
/* Allocates a java byte array from the given block of memory. */
jbyteArray JavaMakeBytes(JNIEnv* env, const cc_uint8* src, cc_uint32 len);
/* Calls a method in the activity class that returns nothing. */
void JavaCallVoid(JNIEnv* env, const char* name, const char* sig, jvalue* args);
/* Calls a method in the activity class that returns a jint. */
jint JavaCallInt(JNIEnv*  env, const char* name, const char* sig, jvalue* args);
/* Calls a method in the activity class that returns a jint. */
jfloat JavaCallFloat(JNIEnv*  env, const char* name, const char* sig, jvalue* args);
/* Calls a method in the activity class that returns a jobject. */
jobject JavaCallObject(JNIEnv* env, const char* name, const char* sig, jvalue* args);
/* Calls a method in the activity class that takes a string and returns nothing. */
void JavaCall_String_Void(const char* name, const String* value);
/* Calls a method in the activity class that takes no arguments and returns a string. */
void JavaCall_Void_String(const char* name, String* dst);
#endif
#endif
