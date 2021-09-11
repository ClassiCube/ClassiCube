#include "Logger.h"
#include "String.h"
#include "Platform.h"
#include "Window.h"
#include "Constants.h"
#include "Game.h"
#include "Funcs.h"
#include "Utils.h"
#include "Launcher.h"
#include "Server.h"
#include "Options.h"

static void RunGame(void) {
	cc_string title; char titleBuffer[STRING_SIZE];
	int width  = Options_GetInt(OPT_WINDOW_WIDTH,  0, DisplayInfo.Width,  0);
	int height = Options_GetInt(OPT_WINDOW_HEIGHT, 0, DisplayInfo.Height, 0);

	/* No custom resolution has been set */
	if (width == 0 || height == 0) {
		width = 854; height = 480;
		if (DisplayInfo.Width < 854) width = 640;
	}

	String_InitArray(title, titleBuffer);
	String_Format2(&title, "%c (%s)", GAME_APP_TITLE, &Game_Username);
	Game_Run(width, height, &title);
}

/* Shows a warning dialog due to an invalid command line argument */
CC_NOINLINE static void WarnInvalidArg(const char* name, const cc_string* arg) {
	cc_string tmp; char tmpBuffer[256];
	String_InitArray(tmp, tmpBuffer);
	String_Format2(&tmp, "%c '%s'", name, arg);

	Logger_DialogTitle = "Failed to start";
	Logger_DialogWarn(&tmp);
}

/* Shows a warning dialog due to insufficient command line arguments */
CC_NOINLINE static void WarnMissingArgs(int argsCount, const cc_string* args) {
	cc_string tmp; char tmpBuffer[256];
	int i;
	String_InitArray(tmp, tmpBuffer);

	String_AppendConst(&tmp, "Missing IP and/or port - ");
	for (i = 0; i < argsCount; i++) { 
		String_AppendString(&tmp, &args[i]);
		String_Append(&tmp, ' ');
	}

	Logger_DialogTitle = "Failed to start";
	Logger_DialogWarn(&tmp);
}

#ifdef CC_BUILD_ANDROID
/* Needs to be externally visible as this is called by Launcher_Run */
int Program_Run(int argc, char** argv) {
#else
static int Program_Run(int argc, char** argv) {
#endif
	cc_string args[GAME_MAX_CMDARGS];
	cc_uint16 port;

	int argsCount = Platform_GetCommandLineArgs(argc, argv, args);
#ifdef _MSC_VER
	/* NOTE: Make sure to comment this out before pushing a commit */
	//cc_string rawArgs = String_FromConst("UnknownShadow200 fffff 127.0.0.1 25565");
	//cc_string rawArgs = String_FromConst("UnknownShadow200"); 
	//argsCount = String_UNSAFE_Split(&rawArgs, ' ', args, 4);
#endif

	if (argsCount == 0) {
#ifdef CC_BUILD_WEB
		String_AppendConst(&Game_Username, "WebTest!");
		RunGame();
#else
		Launcher_Run();
	/* :hash to auto join server with the given hash */
	} else if (argsCount == 1 && args[0].buffer[0] == ':') {
		args[0] = String_UNSAFE_SubstringAt(&args[0], 1);
		String_Copy(&Launcher_AutoHash, &args[0]);
		Launcher_Run();
#endif
	} else if (argsCount == 1) {
		String_Copy(&Game_Username, &args[0]);
		RunGame();		
	} else if (argsCount < 4) {
		WarnMissingArgs(argsCount, args);
		return 1;
	} else {
		String_Copy(&Game_Username, &args[0]);
		String_Copy(&Game_Mppass,   &args[1]);
		String_Copy(&Server.Address,&args[2]);

		if (!Convert_ParseUInt16(&args[3], &port)) {
			WarnInvalidArg("Invalid port", &args[3]);
			return 1;
		}
		Server.Port = port;
		RunGame();
	}
	return 0;
}

/* NOTE: main_real is used for when compiling with MingW without linking to startup files. */
/*  Normally, the final code produced for "main" is our "main" combined with crt's main */
/*  (mingw-w64-crt/crt/gccmain.c) - alas this immediately crashes the game on startup. */
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
	
	res = Platform_SetDefaultCurrentDirectory(argc, argv);
	if (res) Logger_SysWarn(res, "setting current directory");
	Platform_LogConst("Starting " GAME_APP_NAME " ..");
	String_InitArray(Server.Address, ipBuffer);
	Options_Load();

	res = Program_Run(argc, argv);
	Process_Exit(res);
	return res;
}

#if defined CC_BUILD_IOS
/* ClassiCube is sort of and sort of not the executable */
/*  on iOS - UIKit is responsible for kickstarting the game. */
/* (this is handled interop_ios.m as the code is Objective C) */
#elif defined CC_BUILD_ANDROID
/* ClassiCube is just a native library on android, */
/*  unlike other platforms where it is the executable. */
/* (activity java class is responsible for kickstarting the game) */
static void android_main(void) {
	Platform_LogConst("Main loop started!");
	/* Android client is always built with CC_NOMAIN, because a user who */
	/*  compiled a custom version of the client ran into an issue where  */
	/*  the game would wrongly call main in /system/bin/app_process64    */
	/*  instead of the main located just above here. (see issue #864)    */
	/* So use main_real instead to avoid the issue altogether */
	main_real(0, NULL); 
}

/* Called eventually by the activity java class to actually start the game */
static void JNICALL java_runGameAsync(JNIEnv* env, jobject instance) {
	void* thread;
	App_Instance = (*env)->NewGlobalRef(env, instance);
	/* TODO: Do we actually need to remove that global ref later? */

	Platform_LogConst("Running game async!");
	/* The game must be run on a separate thread, as blocking the */
	/* main UI thread will cause a 'App not responding..' messagebox */
	thread = Thread_Start(android_main);
	Thread_Detach(thread);
}
static const JNINativeMethod methods[] = {
	{ "runGameAsync", "()V", java_runGameAsync }
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
