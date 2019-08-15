#ifndef CC_PLATFORM_H
#define CC_PLATFORM_H
#include "PackedCol.h"
#include "Bitmap.h"
/* Abstracts platform specific memory management, I/O, etc.
   Copyright 2014-2019 ClassiCube | Licensed under BSD-3
*/
struct DrawTextArgs;
struct HttpRequest;
struct DateTime;

enum Socket_PollMode { SOCKET_POLL_READ, SOCKET_POLL_WRITE };
#ifdef CC_BUILD_WIN
typedef uintptr_t SocketHandle;
typedef void* FileHandle;
#define _NL "\r\n"
#define UPDATE_FILENAME "update.bat"
#else
typedef int SocketHandle;
typedef int FileHandle;
#define _NL "\n"
#define UPDATE_FILENAME "update.sh"
#endif

/* Origin points for when seeking in a file. */
enum File_SeekFrom { FILE_SEEKFROM_BEGIN, FILE_SEEKFROM_CURRENT, FILE_SEEKFROM_END };
/* Number of milliseconds since 01/01/0001 to start of unix time. */
#define UNIX_EPOCH 62135596800000ULL

extern const ReturnCode ReturnCode_FileShareViolation;
extern const ReturnCode ReturnCode_FileNotFound;
extern const ReturnCode ReturnCode_SocketInProgess;
extern const ReturnCode ReturnCode_SocketWouldBlock;

/* Encodes a string in platform specific format. (e.g. unicode on windows, UTF8 on linux) */
/* NOTE: Only useful for platform specific function calls - do NOT try to interpret the data. */
/* Returns the number of bytes written, excluding trailing NULL terminator. */
CC_API int Platform_ConvertString(void* data, const String* src);
/* Encodes a unicode string in platform specific format. (e.g. unicode on windows, UTF8 on linux) */
/* NOTE: Only useful for platform specific function calls - do NOT try to interpret the data. */
/* Returns the number of bytes written, excluding trailing NULL terminator. */
CC_API int Platform_ConvertUniString(void* data, const UniString* src);

/* Initalises the platform specific state. */
void Platform_Init(void);
/* Frees the platform specific state. */
void Platform_Free(void);
/* Sets the appropriate default current/working directory. */
ReturnCode Platform_SetDefaultCurrentDirectory(void);

/* Gets the command line arguments passed to the program. */
int Platform_GetCommandLineArgs(int argc, STRING_REF char** argv, String* args);
/* Encrypts data in a platform-specific manner. (may not be supported) */
/* NOTE: Should only be implemented when platform natively supports it. */
ReturnCode Platform_Encrypt(const void* data, int len, uint8_t** enc, int* encLen);
/* Decrypts data in a platform-specific manner. (may not be supported) */
/* NOTE: Should only be implemented when platform natively supports it. */
ReturnCode Platform_Decrypt(const void* data, int len, uint8_t** dec, int* decLen);
/* Outputs more detailed information about errors with operating system functions. */
/* NOTE: This is for general functions like file I/O. If a more specific 
describe exists (e.g. DynamicLib_DescribeError), that should be preferred. */
bool Platform_DescribeError(ReturnCode res, String* dst);

/* Starts the given program with the given arguments. */
CC_API ReturnCode Process_Start(const String* path, const String* args);
/* Terminates the process with the given return code. */
CC_API void Process_Exit(ReturnCode code);
/* Starts the platform-specific program to open the given url or filename. */
/* For example, provide a http:// url to open a website in the user's web browser. */
CC_API ReturnCode Process_StartOpen(const String* args);
/* Starts the platform-specific shell to execute the script in UPDATE_FILENAME. */
CC_API ReturnCode Process_StartShell(void);
/* Returns the full path of the application's executable. */
CC_API ReturnCode Process_GetExePath(String* path);

/* Attempts to load a native dynamic library from the given path. */
CC_API ReturnCode DynamicLib_Load(const String* path, void** lib);
/* Attempts to get the address of the symbol in the given dynamic library. */
/* NOTE: Do NOT use this to load OpenGL functions, use GLContext_GetAddress. */
CC_API ReturnCode DynamicLib_Get(void* lib, const char* name, void** symbol);
/* Simple wrapper for DynamicLib_Load then DynamicLib_Get. */
/* NOTE: This should ONLY be used for dynamically loading platform-specific symbols. */
/* (e.g. functionality that only exists on recent operating system versions) */
void* DynamicLib_GetFrom(const char* filename, const char* name);
/* Outputs more detailed information about errors with the DynamicLib functions. */
/* NOTE: You must call this immediately after DynamicLib_Load/DynamicLib_Get,
 because on some platforms, the error is a static string instead of from error code. */
bool DynamicLib_DescribeError(ReturnCode res, String* dst);

/* Allocates a block of memory, with undetermined contents. Returns NULL on allocation failure. */
CC_API void* Mem_TryAlloc(uint32_t numElems, uint32_t elemsSize);
/* Allocates a block of memory, with undetermined contents. Exits process on allocation failure. */
CC_API void* Mem_Alloc(uint32_t numElems, uint32_t elemsSize, const char* place);
/* Allocates a block of memory, with contents of all 0. Exits process on allocation failure. */
CC_API void* Mem_AllocCleared(uint32_t numElems, uint32_t elemsSize, const char* place);
/* Reallocates a block of memory, with undetermined contents. Exits process on reallocation failure. */
CC_API void* Mem_Realloc(void* mem, uint32_t numElems, uint32_t elemsSize, const char* place);
/* Frees an allocated a block of memory. Does nothing when passed NULL. */
CC_API void  Mem_Free(void* mem);
/* Sets the contents of a block of memory to the given value. */
void Mem_Set(void* dst, uint8_t value, uint32_t numBytes);
/* Copies a block of memory to another block of memory. */
/* NOTE: These blocks MUST NOT overlap. */
void Mem_Copy(void* dst, const void* src, uint32_t numBytes);

/* Logs a debug message to console. */
void Platform_Log(const String* message);
void Platform_LogConst(const char* message);
void Platform_Log1(const char* format, const void* a1);
void Platform_Log2(const char* format, const void* a1, const void* a2);
void Platform_Log3(const char* format, const void* a1, const void* a2, const void* a3);
void Platform_Log4(const char* format, const void* a1, const void* a2, const void* a3, const void* a4);

/* Returns the current UTC time, as number of milliseconds since 1/1/0001 */
CC_API TimeMS DateTime_CurrentUTC_MS(void);
/* Returns the current UTC Time. */
/* NOTE: Generally DateTime_CurrentUTC_MS should be used instead. */
CC_API void DateTime_CurrentUTC(struct DateTime* time);
/* Returns the current local Time. */
CC_API void DateTime_CurrentLocal(struct DateTime* time);
/* Takes a platform-specific stopwatch measurement. */
/* NOTE: The value returned is platform-specific - do NOT try to interpret the value. */
CC_API uint64_t Stopwatch_Measure(void);
/* Returns total elapsed microseconds between two stopwatch measurements. */
CC_API uint64_t Stopwatch_ElapsedMicroseconds(uint64_t beg, uint64_t end);

/* Returns whether the given directory exists. */
CC_API bool Directory_Exists(const String* path);
/* Attempts to create a new directory. */
CC_API ReturnCode Directory_Create(const String* path);
/* Callback function invoked for each file found. */
typedef void Directory_EnumCallback(const String* filename, void* obj);
/* Invokes a callback function on all filenames in the given directory (and its sub-directories) */
CC_API ReturnCode Directory_Enum(const String* path, void* obj, Directory_EnumCallback callback);

/* Returns whether the given file exists. */
CC_API bool File_Exists(const String* path);
/* Returns the last time the file was modified, as number of milliseconds since 1/1/0001 */
CC_API ReturnCode File_GetModifiedTime(const String* path, TimeMS* ms);
/* Sets the last time the file was modified, as number of milliseconds since 1/1/0001 */
CC_API ReturnCode File_SetModifiedTime(const String* path, TimeMS ms);
/* Marks a file as being executable. */
CC_API ReturnCode File_MarkExecutable(const String* path);

/* Attempts to create a new (or overwrite) file for writing. */
/* NOTE: If the file already exists, its contents are discarded. */
ReturnCode File_Create(FileHandle* file, const String* path);
/* Attempts to open an existing file for reading. */
ReturnCode File_Open(FileHandle* file, const String* path);
/* Attempts to open (or create) a file, for appending data to the end of the file. */
ReturnCode File_Append(FileHandle* file, const String* path);
/* Attempts to read data from the file. */
ReturnCode File_Read(FileHandle file, uint8_t* data, uint32_t count, uint32_t* bytesRead);
/* Attempts to write data to the file. */
ReturnCode File_Write(FileHandle file, const uint8_t* data, uint32_t count, uint32_t* bytesWrote);
/* Attempts to close the given file. */
ReturnCode File_Close(FileHandle file);
/* Attempts to seek to a position in the given file. */
ReturnCode File_Seek(FileHandle file, int offset, int seekType);
/* Attempts to get the current position in the given file. */
ReturnCode File_Position(FileHandle file, uint32_t* pos);
/* Attempts to retrieve the length of the given file. */
ReturnCode File_Length(FileHandle file, uint32_t* len);

/* Blocks the current thread for the given number of milliseconds. */
CC_API void Thread_Sleep(uint32_t milliseconds);
typedef void Thread_StartFunc(void);
/* Starts a new thread, optionally immediately detaching it. (See Thread_Detach) */
CC_API void* Thread_Start(Thread_StartFunc* func, bool detach);
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
CC_API void  Waitable_WaitFor(void* handle, uint32_t milliseconds);

/* Gets the list of all supported font names on this platform. */
void Font_GetNames(StringsBuffer* buffer);
/* Finds the path and face number of the given font, with closest matching style */
String Font_Lookup(const String* fontName, int style);
/* Allocates a new font from the given arguments. */
ReturnCode Font_Make(FontDesc* desc, const String* fontName, int size, int style);
/* Frees an allocated font. */
CC_API void Font_Free(FontDesc* desc);
/* Measures width of the given text when drawn with the given font. */
int Platform_TextWidth(struct DrawTextArgs* args);
/* Measures height of any text when drawn with the given font. */
int Platform_FontHeight(const FontDesc* font);
/* Draws the given text with the given font onto the given bitmap. */
int Platform_TextDraw(struct DrawTextArgs* args, Bitmap* bmp, int x, int y, BitmapCol col, bool shadow);

/* Allocates a new socket. */
CC_API void Socket_Create(SocketHandle* socket);
/* Returns how much data is available to be read from the given socket. */
CC_API ReturnCode Socket_Available(SocketHandle socket, uint32_t* available);
/* Sets whether operations on the given socket block the calling thread. */
/* e.g. if blocking is on, calling Connect() blocks until a response is received. */
CC_API ReturnCode Socket_SetBlocking(SocketHandle socket, bool blocking);
/* Returns (and resets) the last error generated by the given socket. */
CC_API ReturnCode Socket_GetError(SocketHandle socket, ReturnCode* result);

/* Attempts to initalise a connection to the given IP address:port. */
CC_API ReturnCode Socket_Connect(SocketHandle socket, const String* ip, int port);
/* Attempts to read data from the given socket. */
CC_API ReturnCode Socket_Read(SocketHandle socket, uint8_t* data, uint32_t count, uint32_t* modified);
/* Attempts to write data to the given socket. */
CC_API ReturnCode Socket_Write(SocketHandle socket, const uint8_t* data, uint32_t count, uint32_t* modified);
/* Attempts to close the given socket. */
CC_API ReturnCode Socket_Close(SocketHandle socket);
/* Attempts to poll the given socket for readability or writability. */
/* NOTE: A closed socket is still considered readable. */
/* NOTE: A socket is considered writable once it has finished connecting. */
CC_API ReturnCode Socket_Poll(SocketHandle socket, int mode, bool* success);

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
jbyteArray JavaMakeBytes(JNIEnv* env, const uint8_t* src, uint32_t len);
/* Calls a method in the activity class that returns nothing. */
void JavaCallVoid(JNIEnv* env, const char* name, const char* sig, jvalue* args);
/* Calls a method in the activity class that returns a jint. */
jint JavaCallInt(JNIEnv*  env, const char* name, const char* sig, jvalue* args);
/* Calls a method in the activity class that returns a jobject. */
jobject JavaCallObject(JNIEnv* env, const char* name, const char* sig, jvalue* args);
/* Calls a method in the activity class that takes a string and returns nothing. */
void JavaCall_String_Void(const char* name, const String* value);
/* Calls a method in the activity class that takes no arguments and returns a string. */
void JavaCall_Void_String(const char* name, String* dst);
#endif
#endif
