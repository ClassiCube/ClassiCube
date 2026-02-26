#include "Core.h"
#if defined CC_BUILD_BEOS || defined CC_BUILD_HAIKU
extern "C" {
#include "Platform.h"
#include "String_.h"
#include "Funcs.h"
#include "Utils.h"
}
#include <errno.h>
#include <OS.h>
#include <Roster.h>

/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
cc_uint64 Stopwatch_Measure(void) {
	return system_time();
}

cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;
	return end - beg;
}

cc_result Process_StartOpen(const cc_string* args) {
	static const cc_string https_protocol = String_FromConst("https://");
	char str[NATIVE_STR_LEN];
	String_EncodeUtf8(str, args);

	cc_bool https    = String_CaselessStarts(args, &https_protocol);
	const char* mime = https ? "application/x-vnd.Be.URL.https" : "application/x-vnd.Be.URL.http";
	
	char* argv[] = { str, NULL };
	return be_roster->Launch(mime, 1, argv);
}


/*########################################################################################################################*
*-----------------------------------------------------BeOS threading------------------------------------------------------*
*#########################################################################################################################*/
// NOTE: BeOS only, as haiku uses the more efficient pthreads implementation in Platform_Posix.c
#if defined CC_BUILD_BEOS
void Thread_Sleep(cc_uint32 milliseconds) { snooze(milliseconds * 1000); }

static int32 ExecThread(void* param) {
	((Thread_StartFunc)param)();
	return 0;
}

void Thread_Run(void** handle, Thread_StartFunc func, int stackSize, const char* name) {
	thread_id thread = spawn_thread(ExecThread, name, B_NORMAL_PRIORITY, func);
	*handle = (void*)thread;
	
	resume_thread(thread);
}

void Thread_Detach(void* handle) { }

void Thread_Join(void* handle) {
	thread_id thread = (thread_id)handle;
	wait_for_thread(thread, NULL);
}
#endif


/*########################################################################################################################*
*-----------------------------------------------------Synchronisation-----------------------------------------------------*
*#########################################################################################################################*/
// NOTE: BeOS only, as haiku uses the more efficient pthreads implementation in Platform_Posix.c
#if defined CC_BUILD_BEOS
void* Mutex_Create(const char* name) {
	sem_id id = create_sem(1, name);
	return (void*)id;
}

void Mutex_Free(void* handle) {
	sem_id id = (sem_id)handle;
	delete_sem(id);
}

void Mutex_Lock(void* handle) {
	sem_id id = (sem_id)handle;
	acquire_sem(id);
}

void Mutex_Unlock(void* handle) {
	sem_id id = (sem_id)handle;
	release_sem(id);
}

void* Waitable_Create(const char* name) {
	sem_id id = create_sem(0, name);
	return (void*)id;
}

void Waitable_Free(void* handle) {
	sem_id id = (sem_id)handle;
	delete_sem(id);
}

void Waitable_Signal(void* handle) {
	sem_id id = (sem_id)handle;
	release_sem(id);
}

void Waitable_Wait(void* handle) {
	sem_id id = (sem_id)handle;
	acquire_sem(id);
}

void Waitable_WaitFor(void* handle, cc_uint32 milliseconds) {
	int microseconds = milliseconds * 1000;
	sem_id id = (sem_id)handle;
	acquire_sem_etc(id, 1, B_RELATIVE_TIMEOUT, microseconds);
}
#endif
#endif
