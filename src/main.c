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
#include "main.h"

/*########################################################################################################################*
*-------------------------------------------------Complex argument parsing------------------------------------------------*
*#########################################################################################################################*/
cc_bool Resume_Parse(struct ResumeInfo* info, cc_bool full) {
	String_InitArray(info->server, info->_serverBuffer);
	Options_Get(ROPT_SERVER,       &info->server, "");
	String_InitArray(info->user,   info->_userBuffer);
	Options_Get(ROPT_USER,         &info->user, "");

	String_InitArray(info->ip,   info->_ipBuffer);
	Options_Get(ROPT_IP,         &info->ip, "");
	String_InitArray(info->port, info->_portBuffer);
	Options_Get(ROPT_PORT,       &info->port, "");

	if (!full) return true;
	String_InitArray(info->mppass, info->_mppassBuffer);
	Options_GetSecure(ROPT_MPPASS, &info->mppass);

	return 
		info->user.length && info->mppass.length &&
		info->ip.length   && info->port.length;
}

cc_bool DirectUrl_Claims(const cc_string* input, cc_string* addr, cc_string* user, cc_string* mppass) {
	static const cc_string prefix = String_FromConst("mc://");
	cc_string parts[6];
	if (!String_CaselessStarts(input, &prefix)) return false;

	/* mc://[ip:port]/[username]/[mppass] */
	if (String_UNSAFE_Split(input, '/', parts, 6) != 5) return false;

	*addr   = parts[2];
	*user   = parts[3];
	*mppass = parts[4];
	return true;
}

void DirectUrl_ExtractAddress(const cc_string* addr, cc_string* ip, cc_string* port) {
	static const cc_string defPort = String_FromConst("25565");
	int index = String_LastIndexOf(addr, ':');

	/* support either "[IP]" or "[IP]:[PORT]" */
	if (index == -1) {
		*ip   = *addr;
		*port = defPort;
	} else {
		*ip   = String_UNSAFE_Substring(addr, 0, index);
		*port = String_UNSAFE_SubstringAt(addr, index + 1);
	}
}


/*########################################################################################################################*
*------------------------------------------------------Game setup/run-----------------------------------------------------*
*#########################################################################################################################*/
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

static void SetupProgram(int argc, char** argv) {
	static char ipBuffer[STRING_SIZE];
	cc_result res;
	Logger_Hook();
	Window_PreInit();
	Platform_Init();
	
	res = Platform_SetDefaultCurrentDirectory(argc, argv);
	Options_Load();
	Window_Init();
	Gamepads_Init();
	
	if (res) Logger_SysWarn(res, "setting current directory");
	Platform_LogConst("Starting " GAME_APP_NAME " ..");
	String_InitArray(Server.Address, ipBuffer);
}

#define SP_HasDir(path) (String_IndexOf(path, '/') >= 0 || String_IndexOf(path, '\\') >= 0)
static cc_bool IsOpenableFile(const cc_string* path) {
	cc_filepath str;
	if (!SP_HasDir(path)) return false;
	
	Platform_EncodePath(&str, path);
	return File_Exists(&str);
}

static int ParseMPArgs(const cc_string* user, const cc_string* mppass, const cc_string* addr, const cc_string* port) {
	String_Copy(&Game_Username,  user);
	String_Copy(&Game_Mppass,    mppass);
	String_Copy(&Server.Address, addr);

	if (!Convert_ParseInt(port, &Server.Port) || Server.Port < 0 || Server.Port > 65535) {
		WarnInvalidArg("Invalid port", port);
		return false;
	}
	return true;
}

static int RunProgram(int argc, char** argv) {
	cc_string args[GAME_MAX_CMDARGS];
	int argsCount = Platform_GetCommandLineArgs(argc, argv, args);
	struct ResumeInfo r;
	cc_string host;

#ifdef _MSC_VER
	/* NOTE: Make sure to comment this out before pushing a commit */
	//cc_string rawArgs = String_FromConst("UnknownShadow200 fffff 127.0.0.1 25565");
	//cc_string rawArgs = String_FromConst("UnknownShadow200"); 
	//argsCount = String_UNSAFE_Split(&rawArgs, ' ', args, 4);
#endif

	if (argsCount == 0) {
#ifdef CC_BUILD_WEB
		String_AppendConst(&Game_Username, DEFAULT_USERNAME);
		RunGame();
#else
		Launcher_Run();
	/* :[hash] - auto join server with the given hash */
	} else if (argsCount == 1 && args[0].buffer[0] == ':') {
		args[0] = String_UNSAFE_SubstringAt(&args[0], 1);
		String_Copy(&Launcher_AutoHash, &args[0]);
		Launcher_Run();
	/* --resume - try to resume to last server */
	} else if (argsCount == 1 && String_CaselessEqualsConst(&args[0], DEFAULT_RESUME_ARG)) {
		if (!Resume_Parse(&r, true)) {
			WarnInvalidArg("No server to resume to", &args[0]);
			return 1;
		}
	
		if (!ParseMPArgs(&r.user, &r.mppass, &r.ip, &r.port)) return 1;
		RunGame();
	/* --singleplayer' - run singleplayer with default user */
	} else if (argsCount == 1 && String_CaselessEqualsConst(&args[0], DEFAULT_SINGLEPLAYER_ARG)) {
		Options_Get(LOPT_USERNAME, &Game_Username, DEFAULT_USERNAME);
		RunGame();
	/* [file path] - run singleplayer with auto loaded map */
	} else if (argsCount == 1 && IsOpenableFile(&args[0])) {
		Options_Get(LOPT_USERNAME, &Game_Username, DEFAULT_USERNAME);
		String_Copy(&SP_AutoloadMap, &args[0]); /* TODO: don't copy args? */
		RunGame();
#endif
	/* mc://[addr]:[port]/[user]/[mppass] - run multiplayer using direct URL form arguments */
	} else if (argsCount == 1 && DirectUrl_Claims(&args[0], &host, &r.user, &r.mppass)) {
		DirectUrl_ExtractAddress(&host, &r.ip, &r.port);

		if (!ParseMPArgs(&r.user, &r.mppass, &r.ip, &r.port)) return 1;
		RunGame();
	/* [user] - run multiplayer using explicit username */
	} else if (argsCount == 1) {
		String_Copy(&Game_Username, &args[0]);
		RunGame();
	/* 2 to 3 arguments - unsupported at present */
	} else if (argsCount < 4) {
		WarnMissingArgs(argsCount, args);
		return 1;
	/* [user] [mppass] [address] [port] - run multiplayer using explicit arguments */
	} else {
		if (!ParseMPArgs(&args[0], &args[1], &args[2], &args[3])) return 1;
		RunGame();
	}
	return 0;
}

#if defined CC_BUILD_IOS
/* ClassiCube is sort of and sort of not the executable */
/*  on iOS - UIKit is responsible for kickstarting the game. */
/* (this is handled in interop_ios.m as the code is Objective C) */
int ios_main(int argc, char** argv) {
	SetupProgram(argc, argv);
	for (;;) { RunProgram(argc, argv); }
	return 0;
}
#elif defined CC_BUILD_ANDROID
/* ClassiCube is just a native library on android, */
/*  unlike other platforms where it is the executable. */
/* (activity java class is responsible for kickstarting the game,
    see Platform_Android.c for the code that actually calls this) */
void android_main(void) {
	Platform_LogConst("Main loop started!");
	SetupProgram(0, NULL);
	for (;;) { RunProgram(0, NULL); }
}
#elif defined CC_BUILD_CONSOLE
int main(int argc, char** argv) {
	SetupProgram(argc, argv);
	while (Window_Main.Exists) { 
		RunProgram(argc, argv); 
	}
	
	Window_Free();
	return 0;
}
#else
/* NOTE: main_real is used for when compiling with MinGW without linking to startup files. */
/*  Normally, the final code produced for "main" is our "main" combined with crt's main */
/*  (mingw-w64-crt/crt/gccmain.c) - alas this immediately crashes the game on startup. */
/* Using main_real instead and setting main_real as the entrypoint fixes the crash. */
#if defined CC_NOMAIN
int main_real(int argc, char** argv) {
#elif defined CC_BUILD_WEB
/* webclient does some asynchronous initialisation first, then kickstarts the game after that */
int web_main(int argc, char** argv) {
#else 
int main(int argc, char** argv) {
#endif
	cc_result res;
	SetupProgram(argc, argv);

	/* If single process mode, then the loop is launcher -> game -> launcher etc */
	do {
		res = RunProgram(argc, argv);
	} while (Platform_SingleProcess && Window_Main.Exists);

#ifdef CC_BUILD_WEB
	if (res) Window_Free();
#else
	Window_Free();
#endif

	Process_Exit(res);
	return res;
}
#endif
