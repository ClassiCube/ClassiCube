#ifndef CC_PLATFORM_H
#define CC_PLATFORM_H
#include "PackedCol.h"
#include "Bitmap.h"
/* Abstracts platform specific memory management, I/O, etc.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/
struct DrawTextArgs;
struct AsyncRequest;
struct DateTime;

enum Socket_Select { SOCKET_SELECT_READ, SOCKET_SELECT_WRITE };
#ifdef CC_BUILD_WIN
typedef uintptr_t SocketHandle;
typedef void* FileHandle;
#else
typedef int SocketHandle;
typedef int FileHandle;
#endif

/* Origin points for when seeking in a file. */
enum File_SeekFrom { FILE_SEEKFROM_BEGIN, FILE_SEEKFROM_CURRENT, FILE_SEEKFROM_END };
/* Number of milliseconds since 01/01/0001 to start of unix time. */
#define UNIX_EPOCH 62135596800000ULL

/* Newline for console and text files. */
extern char* Platform_NewLine;
/* Name of default system font used. (e.g. Arial) */
extern char* Font_DefaultName;
extern const ReturnCode ReturnCode_FileShareViolation;
extern const ReturnCode ReturnCode_FileNotFound;
extern const ReturnCode ReturnCode_NotSupported;
extern const ReturnCode ReturnCode_SocketInProgess;
extern const ReturnCode ReturnCode_SocketWouldBlock;
extern const ReturnCode ReturnCode_InvalidArg;

/* Data for a display device. (usually a monitor) */
struct DisplayDevice { int BitsPerPixel; Rect2D Bounds; };
extern struct DisplayDevice DisplayDevice_Default;
extern void* DisplayDevice_Meta;

struct GraphicsMode {
	int R,G,B,A, BitsPerPixel, IsIndexed; /* Colour buffer data */
	int DepthBits, StencilBits; /* Z buffer data */
	int Buffers; /* Number of buffers (usually 2 for double buffer) */
};
/* Creates a new GraphicsMode from the given data. */
void GraphicsMode_Make(struct GraphicsMode* m, int bpp, int depth, int stencil, int buffers);
/* Creates a GraphicsMode compatible with the default display device. */
void GraphicsMode_MakeDefault(struct GraphicsMode* m);

/* Encodes a string in platform specific format. (e.g. unicode on windows, UTF8 on linux) */
/* NOTE: Only useful for platform specific function calls - do NOT try to interpret the data. 
Returns the number of bytes written, excluding trailing NULL terminator. */
CC_EXPORT int Platform_ConvertString(void* data, const String* src);
/* Initalises the platform specific state. */
void Platform_Init(void);
/* Frees the platform specific state. */
void Platform_Free(void);
/* Sets current directory to the directory the executable is in. */
void Platform_SetWorkingDir(void);
/* Exits the process with the given return code .*/
void Platform_Exit(ReturnCode code);
/* Gets the command line arguments passed to the program. */
int  Platform_GetCommandLineArgs(int argc, STRING_REF const char** argv, String* args);

/* Starts the given program with the given arguments. */
CC_EXPORT ReturnCode Platform_StartProcess(const String* path, const String* args);
/* Starts the platform-specific program to open the given url or filename. */
/* For example, provide a http:// url to open a website in the user's web browser. */
CC_EXPORT ReturnCode Platform_StartOpen(const String* args);
/* Attempts to load the native dynamic library from the given path. */
CC_EXPORT ReturnCode Platform_LoadLibrary(const String* path, void** lib);
/* Attempts to get the address of the symbol in the given dynamic library. */
/* NOTE: Do NOT use this to load OpenGL functions, use GLContext_GetAddress. */
CC_EXPORT ReturnCode Platform_GetSymbol(void* lib, const char* name, void** symbol);

/* Allocates a block of memory, with undetermined contents. Exits process on allocation failure. */
CC_EXPORT void* Mem_Alloc(uint32_t numElems, uint32_t elemsSize, const char* place);
/* Allocates a block of memory, with contents of all 0. Exits process on allocation failure. */
CC_EXPORT void* Mem_AllocCleared(uint32_t numElems, uint32_t elemsSize, const char* place);
/* Reallocates a block of memory, with undetermined contents. Exits process on reallocation failure. */
CC_EXPORT void* Mem_Realloc(void* mem, uint32_t numElems, uint32_t elemsSize, const char* place);
/* Frees an allocated a block of memory. Does nothing when passed NULL. */
CC_EXPORT void  Mem_Free(void* mem);
/* Sets the contents of a block of memory to the given value. */
void Mem_Set(void* dst, uint8_t value, uint32_t numBytes);
/* Copies a block of memory to another block. NOTE: These blocks MUST NOT overlap. */
void Mem_Copy(void* dst, void* src, uint32_t numBytes);

/* Logs a debug message to console. */
void Platform_Log(const String* message);
void Platform_LogConst(const char* message);
void Platform_Log1(const char* format, const void* a1);
void Platform_Log2(const char* format, const void* a1, const void* a2);
void Platform_Log3(const char* format, const void* a1, const void* a2, const void* a3);
void Platform_Log4(const char* format, const void* a1, const void* a2, const void* a3, const void* a4);

/* Returns the current UTC time, as number of milliseconds since 1/1/0001 */
TimeMS DateTime_CurrentUTC_MS(void);
/* Returns the current UTC Time. */
/* NOTE: Generally DateTime_CurrentUTC_MS should be used instead. */
void DateTime_CurrentUTC(struct DateTime* time);
/* Returns the current local Time. */
void DateTime_CurrentLocal(struct DateTime* time);
/* Takes a platform-specific stopwatch measurement. */
/* NOTE: The value returned is platform-specific - do NOT try to interpret the value. */
uint64_t Stopwatch_Measure(void);
/* Returns total elapsed microseconds between two stopwatch measurements. */
int Stopwatch_ElapsedMicroseconds(uint64_t beg, uint64_t end);

/* Returns whether the given directory exists. */
bool Directory_Exists(const String* path);
/* Attempts to create a new directory. */
ReturnCode Directory_Create(const String* path);
/* Callback function invoked for each file found. */
typedef void Directory_EnumCallback(const String* filename, void* obj);
/* Invokes a callback function on all filenames in the given directory (and its sub-directories) */
ReturnCode Directory_Enum(const String* path, void* obj, Directory_EnumCallback callback);
/* Returns whether the given file exists. */
bool File_Exists(const String* path);
/* Returns the last time the file was modified, as number of milliseconds since 1/1/0001 */
ReturnCode File_GetModifiedTime(const String* path, TimeMS* ms);

/* Attempts to create a new (or overwrite) file for writing. */
ReturnCode File_Create(FileHandle* file, const String* path);
/* Attempts to open an existing file for reading. */
ReturnCode File_Open(FileHandle* file, const String* path);
/* Attempts to open (or create) a file, for appending data to the end of the file. */
ReturnCode File_Append(FileHandle* file, const String* path);
/* Attempts to read data from the file. */
ReturnCode File_Read(FileHandle file, uint8_t* buffer, uint32_t count, uint32_t* bytesRead);
/* Attempts to write data to the file. */
ReturnCode File_Write(FileHandle file, uint8_t* buffer, uint32_t count, uint32_t* bytesWrote);
/* Attempts to close the given file. */
ReturnCode File_Close(FileHandle file);
/* Attempts to seek to a position in the given file. */
ReturnCode File_Seek(FileHandle file, int offset, int seekType);
/* Attempts to get the current position in the given file. */
ReturnCode File_Position(FileHandle file, uint32_t* position);
/* Attempts to retrieve the length of the given file. */
ReturnCode File_Length(FileHandle file, uint32_t* length);

/* Blocks the current thread for the given number of milliseconds. */
CC_EXPORT void Thread_Sleep(uint32_t milliseconds);
typedef void Thread_StartFunc(void);
/* Starts a new thread, optionally immediately detaching it. (See Thread_Detach) */
CC_EXPORT void* Thread_Start(Thread_StartFunc* func, bool detach);
/* Frees the platform specific persistent data associated with the thread. */
/* NOTE: You must either detach or join threads, as this data otherwise leaks. */
CC_EXPORT void Thread_Detach(void* handle);
/* Blocks the current thread, until the given thread has finished. */
/* NOTE: Once a thread has been detached, you can no longer use this method. */
CC_EXPORT void Thread_Join(void* handle);

/* Allocates a new mutex. (used to synchronise access to a shared resource) */
CC_EXPORT void* Mutex_Create(void);
/* Frees an allocated mutex. */
CC_EXPORT void  Mutex_Free(void* handle);
/* Locks the given mutex, blocking other threads from entering. */
CC_EXPORT void  Mutex_Lock(void* handle);
/* Unlocks the given mutex, allowing other threads to enter. */
CC_EXPORT void  Mutex_Unlock(void* handle);

/* Allocates a new waitable. (used to conditionally wake-up a blocked thread) */
CC_EXPORT void* Waitable_Create(void);
/* Frees an allocated waitable. */
CC_EXPORT void  Waitable_Free(void* handle);
/* Signals a waitable, waking up blocked threads. */
CC_EXPORT void  Waitable_Signal(void* handle);
/* Blocks the calling thread until the waitable gets signalled. */
CC_EXPORT void  Waitable_Wait(void* handle);
/* Blocks the calling thread until the waitable gets signalled, or milliseconds delay passes. */
CC_EXPORT void  Waitable_WaitFor(void* handle, uint32_t milliseconds);

/* Gets the list of all supported font names on this platform. */
CC_EXPORT void Font_GetNames(StringsBuffer* buffer);
/* Allocates a new font from the given arguments. */
CC_EXPORT void Font_Make(FontDesc* desc, const String* fontName, int size, int style);
/* Frees an allocated font. */
CC_EXPORT void Font_Free(FontDesc* desc);
/* Measures dimensions of the given text, if it was drawn with the given font. */
CC_EXPORT Size2D Platform_TextMeasure(struct DrawTextArgs* args);
/* Draws the given text with the given font onto the given bitmap. */
CC_EXPORT Size2D Platform_TextDraw(struct DrawTextArgs* args, Bitmap* bmp, int x, int y, BitmapCol col);

/* Allocates a new socket. */
void Socket_Create(SocketHandle* socket);
/* Returns how much data is available to be read from the given socket. */
ReturnCode Socket_Available(SocketHandle socket, uint32_t* available);
/* Sets whether operations on this socket block the calling thread. */
/* e.g. if blocking is on, calling Connect() blocks until a response is received. */
ReturnCode Socket_SetBlocking(SocketHandle socket, bool blocking);
/* Returns (and resets) the last error generated by this socket. */
ReturnCode Socket_GetError(SocketHandle socket, ReturnCode* result);

/* Attempts to initalise a TCP connection to the given IP address:port. */
ReturnCode Socket_Connect(SocketHandle socket, const String* ip, int port);
/* Attempts to read data from the given socket. */
ReturnCode Socket_Read(SocketHandle socket, uint8_t* buffer, uint32_t count, uint32_t* modified);
/* Attempts to write data to the given socket. */
ReturnCode Socket_Write(SocketHandle socket, uint8_t* buffer, uint32_t count, uint32_t* modified);
/* Attempts to close the given socket. */
ReturnCode Socket_Close(SocketHandle socket);
/* Attempts to poll the socket for reading or writing. */
ReturnCode Socket_Select(SocketHandle socket, int selectMode, bool* success);

/* Initalises the platform specific http library state. */
void Http_Init(void);
/* Performs a http request, setting progress as data is received. */
/* NOTE: NOT thread safe - you should ALWAYS use AsyncDownloader for making requests. */
ReturnCode Http_Do(struct AsyncRequest* req, volatile int* progress);
/* Frees the platform specific http library state. */
ReturnCode Http_Free(void);

#define AUDIO_MAX_BUFFERS 4
/* Information about a source of audio. */
struct AudioFormat { uint16_t Channels, BitsPerSample; int SampleRate; };
#define AudioFormat_Eq(a, b) ((a)->Channels == (b)->Channels && (a)->BitsPerSample == (b)->BitsPerSample && (a)->SampleRate == (b)->SampleRate)
typedef int AudioHandle;

/* Allocates an audio context. */
void Audio_Init(AudioHandle* handle, int buffers);
/* Frees an allocated audio context. */
/* NOTE: Audio_StopAndFree should be used, because this method can fail if audio is playing. */
ReturnCode Audio_Free(AudioHandle handle);
/* Stops playing audio, unqueues buffers, then frees the audio context. */
ReturnCode Audio_StopAndFree(AudioHandle handle);
/* Returns the format audio is played in. */
struct AudioFormat* Audio_GetFormat(AudioHandle handle);
/* Sets the format audio to play is in. */
/* NOTE: Changing the format can be expensive, depending on the platform. */
ReturnCode Audio_SetFormat(AudioHandle handle, struct AudioFormat* format);
/* Sets the audio data in the given buffer. */
/* NOTE: You should ensure Audio_IsCompleted returns true before calling this. */
ReturnCode Audio_BufferData(AudioHandle handle, int idx, void* data, uint32_t dataSize);
/* Begins playing audio. Audio_BufferData must have been used before this. */
ReturnCode Audio_Play(AudioHandle handle);
/* Immediately stops the currently playing audio. */
ReturnCode Audio_Stop(AudioHandle handle);
/* Returns whether the given buffer has finished playing. */
ReturnCode Audio_IsCompleted(AudioHandle handle, int idx, bool* completed);
/* Returns whether all buffers have finished playing. */
ReturnCode Audio_IsFinished(AudioHandle handle, bool* finished);
#endif
