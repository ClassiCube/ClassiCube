#include "../Chat.h"
#include "../Constants.h"
#include "../Errors.h"
#include "../Funcs.h"
#include "../String_.h"
#include "../Platform.h"
#include "../Window.h"
#include "../Platform.h"
#include "interop_android.h"

#include <errno.h>
#include <unistd.h>
#include <android/log.h>

static jmethodID JAVA_getApkUpdateTime;
static jmethodID JAVA_shareScreenshot;
static jmethodID JAVA_getGameDataDir;
static jmethodID JAVA_getGameCacheDir;
static jmethodID JAVA_startOpen;
static jmethodID JAVA_getUUID;


/*########################################################################################################################*
*-----------------------------------------------------Main entrypoint-----------------------------------------------------*
*#########################################################################################################################*/
#include "../main_impl.h"

// ClassiCube is just a native library on android,
//   unlike most other platforms where it is the executable.
// The activity java class is responsible for kickstarting the game
void android_main(void) {
	Platform_LogConst("Main loop started!");
	SetupProgram(0, NULL);
	for (;;) { RunProgram(0, NULL); }
}


/*########################################################################################################################*
*------------------------------------------------------Logging/Time-------------------------------------------------------*
*#########################################################################################################################*/
void Platform_Log(const char* msg, int len) {
	char tmp[2048 + 1];
	len = min(len, 2048);

	Mem_Copy(tmp, msg, len); tmp[len] = '\0';
	/* log using logchat */
	__android_log_write(ANDROID_LOG_DEBUG, "ClassiCube", tmp);
}


/*########################################################################################################################*
*-----------------------------------------------------Process/Module------------------------------------------------------*
*#########################################################################################################################*/
cc_result Process_StartOpen(const cc_string* open_args) {
	JNIEnv* env;
	jvalue args[1];
	Java_GetCurrentEnv(env);

	args[0].l = Java_AllocString(env, open_args);
	Java_ICall_Void(env, JAVA_startOpen, args);
	
	Java_DeleteLocalRef(env, args[0].l);
	return 0;
}


/*########################################################################################################################*
*--------------------------------------------------------Updater----------------------------------------------------------*
*#########################################################################################################################*/
const struct UpdaterInfo Updater_Info = {
	"&eRedownload and reinstall to update", 0
};
cc_bool Updater_Clean(void) { return true; }

cc_result Updater_GetBuildTime(cc_uint64* t) {
	JNIEnv* env;
	Java_GetCurrentEnv(env);
	
	*t = Java_ICall_Long(env, JAVA_getApkUpdateTime, NULL);
	return 0;
}

cc_result Updater_Start(const char** action)   { *action = "Updating game"; return ERR_NOT_SUPPORTED; }
cc_result Updater_MarkExecutable(void)         { return 0; }
cc_result Updater_SetNewBuildTime(cc_uint64 t) { return ERR_NOT_SUPPORTED; }


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
void Platform_TryLogJavaError(void) {
	JNIEnv* env;
	jthrowable err;
	Java_GetCurrentEnv(env);

	err = (*env)->ExceptionOccurred(env);
	if (!err) return;

	Platform_LogConst("PANIC");
	(*env)->ExceptionDescribe(env);
	(*env)->ExceptionClear(env);
	/* TODO actually do something */
}

void Platform_ShareScreenshot(const cc_string* filename) {
	cc_string errMsg; char buffer[FILENAME_SIZE];
	String_InitArray(errMsg, buffer);
	
	JNIEnv* env;
	jobject obj;
	jvalue args[1];
	Java_GetCurrentEnv(env);

	args[0].l = Java_AllocString(env, filename);
	obj       = Java_ICall_Obj(env, JAVA_shareScreenshot, args);
	Java_DecodeString(env, obj, &errMsg);
	
	Java_DeleteLocalRef(env, args[0].l);
	Java_DeleteLocalRef(env, obj);

	if (!errMsg.length) return;
	Chat_AddRaw("&cError sharing screenshot");
	Chat_Add1("  &c%s", &errMsg);
}

void Directory_GetCachePath(cc_string* path) {
	JNIEnv* env;
	Java_GetCurrentEnv(env);

	jobject obj = Java_ICall_Obj(env, JAVA_getGameCacheDir, NULL);
	Java_DecodeString(env, obj, path);
	
	Java_DeleteLocalRef(env, obj);
}

/* All threads using JNI must detach BEFORE they exit */
/* (see https://developer.android.com/training/articles/perf-jni#threads */
void* ExecThread(void* param) {
	JNIEnv* env;
	Java_GetCurrentEnv(env);

	((Thread_StartFunc)param)();
	(*VM_Ptr)->DetachCurrentThread(VM_Ptr);
	return NULL;
}


/*########################################################################################################################*
*-----------------------------------------------------Configuration-------------------------------------------------------*
*#########################################################################################################################*/
cc_result Platform_SetDefaultCurrentDirectory(int argc, char **argv) {
	cc_string dir; char dirBuffer[FILENAME_SIZE + 1];
	String_InitArray_NT(dir, dirBuffer);
	JNIEnv* env;
	Java_GetCurrentEnv(env);

	jobject obj = Java_ICall_Obj(env, JAVA_getGameDataDir, NULL);
	Java_DecodeString(env, obj, &dir);
	
	Java_DeleteLocalRef(env, obj);
	// TODO should be raw path
	dir.buffer[dir.length] = '\0';
	Platform_Log1("DATA DIR: %s|", &dir);

	int res = chdir(dir.buffer) == -1 ? errno : 0;
	if (!res) return 0;

	// show the path to the user
	// TODO there must be a better way
	Window_ShowDialog("Failed to set working directory to", dir.buffer);
	return res;
}

void GetDeviceUUID(cc_string* str) {
	JNIEnv* env;
	Java_GetCurrentEnv(env);

	jobject obj = Java_ICall_Obj(env, JAVA_getUUID, NULL);
	Java_DecodeString(env, obj, str);
	
	Java_DeleteLocalRef(env, obj);
}


/*########################################################################################################################*
*-----------------------------------------------------Java Interop--------------------------------------------------------*
*#########################################################################################################################*/
jclass  App_Class;
jobject App_Instance;
JavaVM* VM_Ptr;

/* JNI helpers */
cc_string JavaGetString(JNIEnv* env, jstring str, char* buffer) {
	const char* src; int len;
	cc_string dst;
	src = (*env)->GetStringUTFChars(env, str, NULL);
	len = (*env)->GetStringUTFLength(env, str);

	dst.buffer   = buffer;
	dst.length   = 0;
	dst.capacity = NATIVE_STR_LEN;
	String_AppendUtf8(&dst, src, len);

	(*env)->ReleaseStringUTFChars(env, str, src);
	return dst;
}

void Java_DecodeString(JNIEnv* env, jstring str, cc_string* dst) {
	const jchar* src;
	jsize len;
	if (!str) return;

	src = (*env)->GetStringChars(env,  str, NULL);
	len = (*env)->GetStringLength(env, str);
	String_AppendUtf16(dst, src, len * 2);
	(*env)->ReleaseStringChars(env, str, src);
}

jobject Java_AllocString(JNIEnv* env, const cc_string* str) {
	cc_uint8 tmp[2048 + 4];
	cc_uint8* cur;
	int i, len = 0;

	for (i = 0; i < str->length && len < 2048; i++) {
		cur = tmp + len;
		len += Convert_CP437ToUtf8(str->buffer[i], cur);
	}
	tmp[len] = '\0';
	return (*env)->NewStringUTF(env, (const char*)tmp);
}

jbyteArray Java_AllocBytes(JNIEnv* env, const void* src, cc_uint32 len) {
	if (!len) return NULL;
	
	jbyteArray arr = (*env)->NewByteArray(env, len);
	(*env)->SetByteArrayRegion(env, arr, 0, len, src);
	return arr;
}


/*########################################################################################################################*
*----------------------------------------------------Initialisation-------------------------------------------------------*
*#########################################################################################################################*/
static void JNICALL java_updateInstance(JNIEnv* env, jobject instance) {
	Platform_LogConst("App instance updated!");
	App_Instance = (*env)->NewGlobalRef(env, instance);
	/* TODO: Do we actually need to remove that global ref later? */
}

/* Called eventually by the activity java class to actually start the game */
static void JNICALL java_runGameAsync(JNIEnv* env, jobject instance) {
	void* thread;
	java_updateInstance(env, instance);

	Platform_LogConst("Running game async!");
	/* The game must be run on a separate thread, as blocking the */
	/* main UI thread will cause a 'App not responding..' messagebox */
	Thread_Run(&thread, android_main, 1024 * 1024, "Game"); // TODO check stack size needed
	Thread_Detach(thread);
}

static const JNINativeMethod methods[] = {
	{ "updateInstance", "()V", java_updateInstance },
	{ "runGameAsync",   "()V", java_runGameAsync }
};

static void CacheJavaMethodRefs(JNIEnv* env) {
	JAVA_getApkUpdateTime = Java_GetIMethod(env, "getApkUpdateTime",      "()J");
	JAVA_shareScreenshot  = Java_GetIMethod(env, "shareScreenshot",       "(Ljava/lang/String;)Ljava/lang/String;");
	JAVA_getGameDataDir   = Java_GetIMethod(env, "getGameDataDirectory",  "()Ljava/lang/String;");
	JAVA_getGameCacheDir  = Java_GetIMethod(env, "getGameCacheDirectory", "()Ljava/lang/String;");
	JAVA_startOpen        = Java_GetIMethod(env, "startOpen",             "(Ljava/lang/String;)V");
	JAVA_getUUID          = Java_GetIMethod(env, "getUUID",               "()Ljava/lang/String;");
}

/* This method is automatically called by the Java VM when the */
/*  activity java class calls 'System.loadLibrary("classicube");' */
CC_API jint JNI_OnLoad(JavaVM* vm, void* reserved) {
	jclass klass;
	JNIEnv* env;
	VM_Ptr = vm;
	Java_GetCurrentEnv(env);

	klass     = (*env)->FindClass(env, "com/classicube/MainActivity");
	App_Class = (*env)->NewGlobalRef(env, klass);
	
	Java_RegisterNatives(env, methods);
	CacheJavaMethodRefs(env);
	return JNI_VERSION_1_4;
}
