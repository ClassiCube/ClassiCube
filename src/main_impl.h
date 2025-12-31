/* 
NOTE: This file contains the common entrypoint code for ClassiCube.

This file is included by platform backend files (e.g. see Platform_Windows.c, Platform_Posix.c)
This separation is necessary because some platforms require special 'main' functions.

Eg. the webclient 'main' function loads IndexedDB, and when that has asynchronously finished, 
  then calls the 'web_main' callback - which actually runs ClassiCube
*/
#include "Logger.h"
#include "String_.h"
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
	Game_Setup();
	while (Game_Running) { Game_RenderFrame(); }

	Game_Free();
	Window_Destroy();
}

static void RunLauncher(void) {
#ifndef CC_BUILD_WEB
	Launcher_Setup();
	/* NOTE: Make sure to keep delay same as hardcoded delay in Launcher_Tick */
	while (Launcher_Tick()) { Thread_Sleep(10); }

	Launcher_Finish();
	Window_Destroy();
#endif
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
	CrashHandler_Install();
	Logger_Hook();
	Window_PreInit();
	Gamepads_PreInit();
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

#define ARG_RESULT_RUN_LAUNCHER 1
#define ARG_RESULT_RUN_GAME     2
#define ARG_RESULT_INVALID_ARGS 3

static int ProcessProgramArgs(int argc, char** argv) {
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

	if (argsCount == 0)
		return ARG_RESULT_RUN_LAUNCHER;

#ifndef CC_BUILD_WEB
	/* :[hash] - auto join server with the given hash */
	if (argsCount == 1 && args[0].buffer[0] == ':') {
		args[0] = String_UNSAFE_SubstringAt(&args[0], 1);
		String_Copy(&Launcher_AutoHash, &args[0]);
		return ARG_RESULT_RUN_LAUNCHER;
	}

	/* --resume - try to resume to last server */
	if (argsCount == 1 && String_CaselessEqualsConst(&args[0], DEFAULT_RESUME_ARG)) {
		if (!Resume_Parse(&r, true)) {
			WarnInvalidArg("No server to resume to", &args[0]);
			return ARG_RESULT_INVALID_ARGS;
		}
	
		if (!ParseMPArgs(&r.user, &r.mppass, &r.ip, &r.port)) 
			return ARG_RESULT_INVALID_ARGS;
		return ARG_RESULT_RUN_GAME;
	}

	/* --singleplayer' - run singleplayer with default user */
	if (argsCount == 1 && String_CaselessEqualsConst(&args[0], DEFAULT_SINGLEPLAYER_ARG)) {
		Options_Get(LOPT_USERNAME, &Game_Username, DEFAULT_USERNAME);
		return ARG_RESULT_RUN_GAME;
	}

	/* [file path] - run singleplayer with auto loaded map */
	if (argsCount == 1 && IsOpenableFile(&args[0])) {
		Options_Get(LOPT_USERNAME, &Game_Username, DEFAULT_USERNAME);
		String_Copy(&SP_AutoloadMap, &args[0]); /* TODO: don't copy args? */
		return ARG_RESULT_RUN_GAME;
	}
#endif

	/* mc://[addr]:[port]/[user]/[mppass] - run multiplayer using direct URL form arguments */
	if (argsCount == 1 && DirectUrl_Claims(&args[0], &host, &r.user, &r.mppass)) {
		DirectUrl_ExtractAddress(&host, &r.ip, &r.port);

		if (!ParseMPArgs(&r.user, &r.mppass, &r.ip, &r.port))
			return ARG_RESULT_INVALID_ARGS;
		return ARG_RESULT_RUN_GAME;
	}

	/* [user] - run multiplayer using explicit username */
	if (argsCount == 1) {
		String_Copy(&Game_Username, &args[0]);
		return ARG_RESULT_RUN_GAME;
	}
	
	/* 2 to 3 arguments - unsupported at present */
	if (argsCount < 4) {
		WarnMissingArgs(argsCount, args);
		return ARG_RESULT_INVALID_ARGS;
	}

	/* [user] [mppass] [address] [port] - run multiplayer using explicit arguments */
	if (!ParseMPArgs(&args[0], &args[1], &args[2], &args[3]))
		return ARG_RESULT_INVALID_ARGS;
	return ARG_RESULT_RUN_GAME;
}

static int RunProgram(int argc, char** argv) {
	switch (ProcessProgramArgs(argc, argv))
	{
	case ARG_RESULT_RUN_LAUNCHER:
		RunLauncher();
		return 0;
	case ARG_RESULT_RUN_GAME:
		RunGame();
		return 0;
	default:
		return 1;
	}
}

