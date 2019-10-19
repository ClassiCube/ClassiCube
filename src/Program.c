#include "Logger.h"
#include "Platform.h"
#include "Window.h"
#include "Constants.h"
#include "Game.h"
#include "Funcs.h"
#include "Utils.h"
#include "Launcher.h"
#include "Server.h"
#include "Args.h"

/*#define CC_TEST_VORBIS*/
#ifdef CC_TEST_VORBIS
#include "ExtMath.h"
#include "Vorbis.h"

#define VORBIS_N 1024
#define VORBIS_N2 (VORBIS_N / 2)
int main_imdct() {
	float in[VORBIS_N2], out[VORBIS_N], out2[VORBIS_N];
	double delta[VORBIS_N];

	RngState rng;
	Random_Seed(&rng, 2342334);
	struct imdct_state imdct;
	imdct_init(&imdct, VORBIS_N);

	for (int ii = 0; ii < VORBIS_N2; ii++) {
		in[ii] = Random_Float(&rng);
	}

	imdct_slow(in, out, VORBIS_N2);
	imdct_calc(in, out2, &imdct);

	double sum = 0;
	for (int ii = 0; ii < VORBIS_N; ii++) {
		delta[ii] = out2[ii] - out[ii];
		sum += delta[ii];
	}
	int fff = 0;
}
#endif

static void RunGame(void) {
	static const String defPath = String_FromConst("texpacks/default.zip");
	String title; char titleBuffer[STRING_SIZE];
	int width, height;

#ifndef CC_BUILD_WEB
	if (!File_Exists(&defPath)) {
		Window_ShowDialog("Missing file",
			"default.zip is missing, try downloading resources first.\n\nThe game will still run, but without any textures");
	}
#endif

	width  = Options_GetInt(OPT_WINDOW_WIDTH,  0, Display_Bounds.Width,  0);
	height = Options_GetInt(OPT_WINDOW_HEIGHT, 0, Display_Bounds.Height, 0);

	/* No custom resolution has been set */
	if (width == 0 || height == 0) {
		width = 854; height = 480;
		if (Display_Bounds.Width < 854) width = 640;
	}

	String_InitArray(title, titleBuffer);
	String_Format2(&title, "%c (%s)", GAME_APP_TITLE, &Game_Username);
	Game_Run(width, height, &title);
}

/* Terminates the program due to an invalid command line argument */
CC_NOINLINE static void ExitInvalidArg(const char* name, const String* arg) {
	String tmp; char tmpBuffer[256];
	String_InitArray(tmp, tmpBuffer);
	String_Format2(&tmp, "%c '%s'", name, arg);

	Logger_DialogTitle = "Failed to start";
	Logger_DialogWarn(&tmp);
	Process_Exit(1);
}

/* Terminates the program due to insufficient command line arguments */
CC_NOINLINE static void ExitMissingArgs(int argsCount, const String* args) {
	String tmp; char tmpBuffer[256];
	int i;
	String_InitArray(tmp, tmpBuffer);

	String_AppendConst(&tmp, "Missing IP and/or port - ");
	for (i = 0; i < argsCount; i++) { 
		String_AppendString(&tmp, &args[i]);
		String_Append(&tmp, ' ');
	}

	Logger_DialogTitle = "Failed to start";
	Logger_DialogWarn(&tmp);
	Process_Exit(1);
}

#ifdef CC_BUILD_ANDROID
int Program_Run(int argc, char** argv) {
#else
static int Program_Run(int argc, char** argv) {
#endif
	cc_uint8 ip[4];
	cc_uint16 port;
	String hashSubStringContainer;

#ifdef _MSC_VER
	/* NOTE: Make sure to comment this out before pushing a commit */
	//String rawArgs = String_FromConst("-u UnknownShadow200 -s fffff -a 127.0.0.1 -p 25565");
	//String rawArgs = String_FromConst("-u UnknownShadow200");
	//Args_SetCustomArgs(&rawArgs);
#endif

	if (!Args_HasEnoughArgs()) {
		ExitMissingArgs(Args_GetArgsCount(), Args_GetRawArgs());
		return 1;
	}


	if (Args_HasNoArguments()) {
#ifdef CC_BUILD_WEB
		String_AppendConst(&Game_Username, "WebTest!");
		RunGame();
#else
		Launcher_Run();
#endif
#ifndef CC_BUILD_WEB
	} else if (Args_OnlyHasHash()) {
		/* :hash to auto join server with the given hash */
		const String* hash = Args_GetHash();
		if (hash->buffer[0] == ':') {
			hashSubStringContainer = String_UNSAFE_SubstringAt(hash, 1);
			String_Copy(&Game_Hash, &hashSubStringContainer);
			Launcher_Run();
			return 0;
		} else {
			ExitInvalidArg("Hash must begin with a colon", hash);
			return 1;
		}
#endif
	} else if (Args_OnlyHasUsername()) {
		String_Copy(&Game_Username, Args_GetUsername());
		RunGame();
	} else {
		String_Copy(&Game_Username, Args_GetUsername());
		String_Copy(&Game_Mppass, Args_GetMPPass());
		String_Copy(&Server.IP, Args_GetIP());

		if (!Utils_ParseIP(Args_GetIP(), ip)) {
			ExitInvalidArg("Invalid IP", Args_GetIP());
			return 1;
		}
		if (!Convert_ParseUInt16(Args_GetPort(), &port)) {
			ExitInvalidArg("Invalid port", Args_GetPort());
			return 1;
		}
		Server.Port = port;
		RunGame();
	}

	Process_Exit(0);
	return 0;
}

/* NOTE: This is used when compiling with MingW without linking to startup files. */
/* The final code produced for "main" is our "main" combined with crt's main. (mingw-w64-crt/crt/gccmain.c) */
/* This immediately crashes the game on startup. */
/* Using main_real instead and setting main_real as the entrypoint fixes the crash. */
#ifdef CC_NOMAIN
int main_real(int argc, char** argv) {
#else
int main(int argc, char** argv) {
#endif
	static char ipBuffer[STRING_SIZE];
	cc_result res;
	Logger_Hook();
	Platform_Init();
	Window_Init();
	Args_Init(argc, argv);

	if (Args_RequestedSpecificDirectory()) {



	} else {

		res = Platform_SetDefaultCurrentDirectory();

	}

	if (res) Logger_Warn(res, "setting current directory");
#ifdef CC_TEST_VORBIS
	main_imdct();
#endif
	Platform_LogConst("Starting " GAME_APP_NAME " ..");
	String_InitArray(Server.IP, ipBuffer);

	Utils_EnsureDirectory("maps");
	Utils_EnsureDirectory("texpacks");
	Utils_EnsureDirectory("texturecache");
	Utils_EnsureDirectory("plugins");
	Options_Load();

	return Program_Run(argc, argv);
}

/* ClassiCube is just a native library on android, */
/* unlike other platforms where it is the executable. */
/* As such, we have to hook into the java-side activity, */
/* which in its onCreate() calls runGameAsync to */
/* actually run the game on a separate thread. */
#ifdef CC_BUILD_ANDROID
static void android_main(void) {
	Platform_LogConst("Main loop started!");
	main(0, NULL); 
}

static void JNICALL java_runGameAsync(JNIEnv* env, jobject instance) {
	App_Instance = (*env)->NewGlobalRef(env, instance);
	/* TODO: Do we actually need to remove that global ref later? */
	Platform_LogConst("Running game async!");
	Thread_Start(android_main, true);
}

static const JNINativeMethod methods[1] = {
	{ "runGameAsync", "()V", java_runGameAsync }
};

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
