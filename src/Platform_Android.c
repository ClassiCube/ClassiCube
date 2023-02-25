#include "Core.h"
#if defined CC_BUILD_ANDROID
#include "Chat.h"
#include "Constants.h"
#include "Errors.h"
#include "Funcs.h"
#include "String.h"
#include "Platform.h"
#include <errno.h>
#include <unistd.h>
#include <android/log.h>


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
static char gameArgs[GAME_MAX_CMDARGS][STRING_SIZE];
static int gameNumArgs;

cc_result Process_StartGame2(const cc_string* args, int numArgs) {
	for (int i = 0; i < numArgs; i++) {
		String_CopyToRawArray(gameArgs[i], &args[i]);
	}

	gameNumArgs = numArgs;
	return 0;
}

cc_result Process_StartOpen(const cc_string* args) {
	JavaCall_String_Void("startOpen", args);
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
	JavaGetCurrentEnv(env);
	*t = JavaCallLong(env, "getApkUpdateTime", "()J", NULL);
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
	JavaGetCurrentEnv(env);

	err = (*env)->ExceptionOccurred(env);
	if (!err) return;

	Platform_LogConst("PANIC");
	(*env)->ExceptionDescribe(env);
	(*env)->ExceptionClear(env);
	/* TODO actually do something */
}

void Platform_ShareScreenshot(const cc_string* filename) {
	cc_string path; char pathBuffer[FILENAME_SIZE];
	String_InitArray(path, pathBuffer);

	JavaCall_String_String("shareScreenshot", filename, &path);
	if (!path.length) return;

	Chat_AddRaw("&cError sharing screenshot");
	Chat_Add1("  &c%s", &path);
}

void Directory_GetCachePath(cc_string* path) {
	// TODO cache method ID
	JavaCall_Void_String("getGameCacheDirectory", path);
}


/*########################################################################################################################*
*-----------------------------------------------------Configuration-------------------------------------------------------*
*#########################################################################################################################*/
int Platform_GetCommandLineArgs(int argc, STRING_REF char** argv, cc_string* args) {
	int count = gameNumArgs;
	for (int i = 0; i < count; i++) {
		args[i] = String_FromRawArray(gameArgs[i]);
	}

	// clear arguments so after game is closed, launcher is started
    gameNumArgs = 0;
    return count;
}

#include "Window.h"
cc_result Platform_SetDefaultCurrentDirectory(int argc, char **argv) {
	cc_string dir; char dirBuffer[FILENAME_SIZE + 1];
	String_InitArray_NT(dir, dirBuffer);

	JavaCall_Void_String("getGameDataDirectory", &dir);
	dir.buffer[dir.length] = '\0';
	Platform_Log1("DATA DIR: %s|", &dir);

	int res = chdir(dir.buffer) == -1 ? errno : 0;
	if (!res) return 0;

	// show the path to the user
	// TODO there must be a better way
	Window_ShowDialog("Failed to set working directory to", dir.buffer);
	return res;
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

jobject JavaMakeString(JNIEnv* env, const cc_string* str) {
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

jbyteArray JavaMakeBytes(JNIEnv* env, const void* src, cc_uint32 len) {
	if (!len) return NULL;
	jbyteArray arr = (*env)->NewByteArray(env, len);
	(*env)->SetByteArrayRegion(env, arr, 0, len, src);
	return arr;
}

void JavaCallVoid(JNIEnv* env, const char* name, const char* sig, jvalue* args) {
	jmethodID method = (*env)->GetMethodID(env, App_Class, name, sig);
	(*env)->CallVoidMethodA(env, App_Instance, method, args);
}

jlong JavaCallLong(JNIEnv* env, const char* name, const char* sig, jvalue* args) {
	jmethodID method = (*env)->GetMethodID(env, App_Class, name, sig);
	return (*env)->CallLongMethodA(env, App_Instance, method, args);
}

jobject JavaCallObject(JNIEnv* env, const char* name, const char* sig, jvalue* args) {
	jmethodID method = (*env)->GetMethodID(env, App_Class, name, sig);
	return (*env)->CallObjectMethodA(env, App_Instance, method, args);
}

void JavaCall_String_Void(const char* name, const cc_string* value) {
	JNIEnv* env;
	jvalue args[1];
	JavaGetCurrentEnv(env);

	args[0].l = JavaMakeString(env, value);
	JavaCallVoid(env, name, "(Ljava/lang/String;)V", args);
	(*env)->DeleteLocalRef(env, args[0].l);
}

static void ReturnString(JNIEnv* env, jobject obj, cc_string* dst) {
	const jchar* src;
	jsize len;
	if (!obj) return;

	src = (*env)->GetStringChars(env,  obj, NULL);
	len = (*env)->GetStringLength(env, obj);
	String_AppendUtf16(dst, src, len * 2);
	(*env)->ReleaseStringChars(env, obj, src);
	(*env)->DeleteLocalRef(env,     obj);
}

void JavaCall_Void_String(const char* name, cc_string* dst) {
	JNIEnv* env;
	jobject obj;
	JavaGetCurrentEnv(env);

	obj = JavaCallObject(env, name, "()Ljava/lang/String;", NULL);
	ReturnString(env, obj, dst);
}

void JavaCall_String_String(const char* name, const cc_string* arg, cc_string* dst) {
	JNIEnv* env;
	jobject obj;
	jvalue args[1];
	JavaGetCurrentEnv(env);

	args[0].l = JavaMakeString(env, arg);
	obj       = JavaCallObject(env, name, "(Ljava/lang/String;)Ljava/lang/String;", args);
	ReturnString(env, obj, dst);
	(*env)->DeleteLocalRef(env, args[0].l);
}


/*########################################################################################################################*
*----------------------------------------------------Initialisation-------------------------------------------------------*
*#########################################################################################################################*/
extern void android_main(void);
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
	thread = Thread_Create(android_main);
	Thread_Start2(thread, android_main);
	Thread_Detach(thread);
}
static const JNINativeMethod methods[] = {
	{ "updateInstance", "()V", java_updateInstance },
	{ "runGameAsync",   "()V", java_runGameAsync }
};

/* This method is automatically called by the Java VM when the */
/*  activity java class calls 'System.loadLibrary("classicube");' */
CC_API jint JNI_OnLoad(JavaVM* vm, void* reserved) {
	jclass klass;
	JNIEnv* env;
	VM_Ptr = vm;
	JavaGetCurrentEnv(env);

	klass     = (*env)->FindClass(env, "com/classicube/MainActivity");
	App_Class = (*env)->NewGlobalRef(env, klass);
	JavaRegisterNatives(env, methods);
	return JNI_VERSION_1_4;
}
#endif
