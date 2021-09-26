#include "Core.h"
#if defined CC_BUILD_ANDROID
#include "Chat.h"
#include "Constants.h"
#include "Errors.h"
#include "Funcs.h"
#include "String.h"
#include "Platform.h"
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
static char gameArgsBuffer[512];
static cc_string gameArgs = String_FromArray(gameArgsBuffer);

cc_result Process_StartGame(const cc_string* args) {
	String_Copy(&gameArgs, args);
	return 0;
}

cc_result Process_StartOpen(const cc_string* args) {
	JavaCall_String_Void("startOpen", args);
	return 0;
}


/*########################################################################################################################*
*--------------------------------------------------------Updater----------------------------------------------------------*
*#########################################################################################################################*/
const char* const Updater_OGL  = NULL;
const char* const Updater_D3D9 = NULL;
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
void Platform_ShareScreenshot(const cc_string* filename) {
	cc_string path; char pathBuffer[FILENAME_SIZE];
	String_InitArray(path, pathBuffer);

	JavaCall_String_String("shareScreenshot", filename, &path);
	if (!path.length) return;

	Chat_AddRaw("&cError sharing screenshot");
	Chat_Add1("  &c%s", &path);
}


/*########################################################################################################################*
*-----------------------------------------------------Configuration-------------------------------------------------------*
*#########################################################################################################################*/
int Platform_GetCommandLineArgs(int argc, STRING_REF char** argv, cc_string* args) {
	int count = 0;
	if (gameArgs.length) {
		count = String_UNSAFE_Split(&gameArgs, ' ', args, GAME_MAX_CMDARGS);
		/* clear arguments so after game is closed, launcher is started */
		gameArgs.length = 0;
	}
	return count;
}

cc_result Platform_SetDefaultCurrentDirectory(int argc, char **argv) {
	cc_string dir; char dirBuffer[FILENAME_SIZE + 1];
	String_InitArray_NT(dir, dirBuffer);

	JavaCall_Void_String("getExternalAppDir", &dir);
	dir.buffer[dir.length] = '\0';
	Platform_Log1("EXTERNAL DIR: %s|", &dir);
	return chdir(dir.buffer) == -1 ? errno : 0;
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
#endif
