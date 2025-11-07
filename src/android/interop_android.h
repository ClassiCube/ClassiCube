#include <jni.h> /* TODO move to interop file */
extern jclass  App_Class;
extern jobject App_Instance;
extern JavaVM* VM_Ptr;

#define Java_GetCurrentEnv(env) (*VM_Ptr)->AttachCurrentThread(VM_Ptr, &env, NULL)

#define Java_DeleteLocalRef(env, arg) (*env)->DeleteLocalRef(env, arg);

#define Java_RegisterNatives(env, methods) (*env)->RegisterNatives(env, App_Class, methods, Array_Elems(methods));
#define Java_GetIMethod(env, name, sig) (*env)->GetMethodID(env, App_Class, name, sig)
#define Java_GetSMethod(env, name, sig) (*env)->GetStaticMethodID(env, App_Class, name, sig)

#define Java_AllocConst(env, str) (*env)->NewStringUTF(env, str)
/* Allocates a java string from the given string. */
jobject Java_AllocString(JNIEnv* env, const cc_string* str);
/* Allocates a java byte array from the given block of memory. */
jbyteArray Java_AllocBytes(JNIEnv* env, const void* src, cc_uint32 len);

/* Decodes the given java string into a string. */
void Java_DecodeString(JNIEnv* env, jstring str, cc_string* dst);
/* Creates a string from the given java string. buffer must be at least NATIVE_STR_LEN long. */
//cc_string Java_DecodeRaw(JNIEnv* env, jstring str, char* buffer);

/* Calls an instance method in the activity class that returns nothing */
#define Java_ICall_Void(env, method, args) (*env)->CallVoidMethodA(env,  App_Instance, method, args)
/* Calls an instance method in the activity class that returns a jint */
#define Java_ICall_Int(env,  method, args) (*env)->CallIntMethodA(env,   App_Instance, method, args)
/* Calls an instance method in the activity class that returns a jlong */
#define Java_ICall_Long(env, method, args) (*env)->CallLongMethodA(env,  App_Instance, method, args)
/* Calls an instance method in the activity class that returns a jfloat */
#define Java_ICall_Float(env,method, args) (*env)->CallFloatMethodA(env, App_Instance, method, args)
/* Calls an instance method in the activity class that returns a jobject */
#define Java_ICall_Obj(env,  method, args) (*env)->CallObjectMethodA(env,App_Instance, method, args)

/* Calls a static method in the activity class that returns nothing */
#define Java_SCall_Void(env, method, args) (*env)->CallStaticVoidMethodA(env,  App_Class, method, args)
/* Calls a static method in the activity class that returns a jint */
#define Java_SCall_Int(env,  method, args) (*env)->CallStaticIntMethodA(env,   App_Class, method, args)
/* Calls a static method in the activity class that returns a jlong */
#define Java_SCall_Long(env, method, args) (*env)->CallStaticLongMethodA(env,  App_Class, method, args)
/* Calls a static method in the activity class that returns a jfloat */
#define Java_SCall_Float(env,method, args) (*env)->CallStaticFloatMethodA(env, App_Class, method, args)
/* Calls a static method in the activity class that returns a jobject */
#define Java_SCall_Obj(env,  method, args) (*env)->CallStaticObjectMethodA(env,App_Class, method, args)