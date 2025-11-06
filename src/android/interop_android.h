#include <jni.h> /* TODO move to interop file */
extern jclass  App_Class;
extern jobject App_Instance;
extern JavaVM* VM_Ptr;

#define JavaGetCurrentEnv(env) (*VM_Ptr)->AttachCurrentThread(VM_Ptr, &env, NULL)
#define JavaMakeConst(env, str) (*env)->NewStringUTF(env, str)

#define JavaRegisterNatives(env, methods) (*env)->RegisterNatives(env, App_Class, methods, Array_Elems(methods));
#define JavaGetIMethod(env, name, sig) (*env)->GetMethodID(env, App_Class, name, sig)
#define JavaGetSMethod(env, name, sig) (*env)->GetStaticMethodID(env, App_Class, name, sig)

/* Creates a string from the given java string. buffer must be at least NATIVE_STR_LEN long. */
/* NOTE: Don't forget to call env->ReleaseStringUTFChars. Only works with ASCII strings. */
cc_string JavaGetString(JNIEnv* env, jstring str, char* buffer);
/* Allocates a java string from the given string. */
jobject JavaMakeString(JNIEnv* env, const cc_string* str);
/* Allocates a java byte array from the given block of memory. */
jbyteArray JavaMakeBytes(JNIEnv* env, const void* src, cc_uint32 len);
/* Calls a method in the activity class that returns nothing. */
void JavaCallVoid(JNIEnv*  env, const char* name, const char* sig, jvalue* args);
/* Calls a method in the activity class that returns a jint. */
jlong JavaCallLong(JNIEnv* env, const char* name, const char* sig, jvalue* args);
/* Calls a method in the activity class that returns a jobject. */
jobject JavaCallObject(JNIEnv* env, const char* name, const char* sig, jvalue* args);
/* Calls a method in the activity class that takes a string and returns nothing. */
void JavaCall_String_Void(const char* name, const cc_string* value);
/* Calls a method in the activity class that takes no arguments and returns a string. */
void JavaCall_Void_String(const char* name, cc_string* dst);
/* Calls a method in the activity class that takes a string and returns a string. */
void JavaCall_String_String(const char* name, const cc_string* arg, cc_string* dst);

/* Calls an instance method in the activity class that returns nothing */
#define JavaICall_Void(env, method, args) (*env)->CallVoidMethodA(env,  App_Instance, method, args)
/* Calls an instance method in the activity class that returns a jint */
#define JavaICall_Int(env,  method, args) (*env)->CallIntMethodA(env,   App_Instance, method, args)
/* Calls an instance method in the activity class that returns a jlong */
#define JavaICall_Long(env, method, args) (*env)->CallLongMethodA(env,  App_Instance, method, args)
/* Calls an instance method in the activity class that returns a jfloat */
#define JavaICall_Float(env,method, args) (*env)->CallFloatMethodA(env, App_Instance, method, args)
/* Calls an instance method in the activity class that returns a jobject */
#define JavaICall_Obj(env,  method, args) (*env)->CallObjectMethodA(env,App_Instance, method, args)

/* Calls a static method in the activity class that returns nothing */
#define JavaSCall_Void(env, method, args) (*env)->CallStaticVoidMethodA(env,  App_Class, method, args)
/* Calls a static method in the activity class that returns a jint */
#define JavaSCall_Int(env,  method, args) (*env)->CallStaticIntMethodA(env,   App_Class, method, args)
/* Calls a static method in the activity class that returns a jlong */
#define JavaSCall_Long(env, method, args) (*env)->CallStaticLongMethodA(env,  App_Class, method, args)
/* Calls a static method in the activity class that returns a jfloat */
#define JavaSCall_Float(env,method, args) (*env)->CallStaticFloatMethodA(env, App_Class, method, args)
/* Calls a static method in the activity class that returns a jobject */
#define JavaSCall_Obj(env,  method, args) (*env)->CallStaticObjectMethodA(env,App_Class, method, args)