#include "Logger.h"
#include "Platform.h"
#include "Window.h"
#include "Constants.h"
#include "Game.h"
#include "Funcs.h"
#include "Utils.h"
#include "Launcher.h"
#include "Server.h"

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

static void Program_RunGame(void) {
	const static String defPath = String_FromConst("texpacks/default.zip");
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

/* Attempts to set current/working directory to the directory exe file is in */
static void Program_SetCurrentDirectory(void) {
	String path; char pathBuffer[FILENAME_SIZE];
	int i;
	ReturnCode res;
	String_InitArray(path, pathBuffer);

#ifdef CC_BUILD_WEB
	String_AppendConst(&path, "/classicube");
	res = Platform_SetCurrentDirectory(&path);
	if (res) { Logger_Warn(res, "setting current directory"); return; }
#else
	res = Process_GetExePath(&path);
	if (res) { Logger_Warn(res, "getting exe path"); return; }

	/* get rid of filename at end of directory */
	for (i = path.length - 1; i >= 0; i--, path.length--) {
		if (path.buffer[i] == '/' || path.buffer[i] == '\\') break;
	}
	res = Platform_SetCurrentDirectory(&path);
	if (res) { Logger_Warn(res, "setting current directory"); return; }
#endif
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

int main(int argc, char** argv) {
	static char ipBuffer[STRING_SIZE];
	String args[GAME_MAX_CMDARGS];
	int argsCount;
	uint8_t ip[4];
	uint16_t port;

	Logger_Hook();
	Platform_Init();
	Window_Init();
	Program_SetCurrentDirectory();
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

	argsCount = Platform_GetCommandLineArgs(argc, argv, args);
	/* NOTE: Make sure to comment this out before pushing a commit */
	/* String rawArgs = String_FromConst("UnknownShadow200 fffff 127.0.0.1 25565"); */
	/* String rawArgs = String_FromConst("UnknownShadow200"); */
	/* argsCount = String_UNSAFE_Split(&rawArgs, ' ', args, 4); */

	if (argsCount == 0) {
#ifdef CC_BUILD_WEB
		String_AppendConst(&Game_Username, "WebTest!");
		Program_RunGame();
#else
		Launcher_Run();
#endif
	} else if (argsCount == 1) {
#ifndef CC_BUILD_WEB
		/* :hash to auto join server with the given hash */
		if (args[0].buffer[0] == ':') {
			args[0] = String_UNSAFE_SubstringAt(&args[0], 1);
			String_Copy(&Game_Hash, &args[0]);
			Launcher_Run();
			return 0;
		}
#endif
		String_Copy(&Game_Username, &args[0]);
		Program_RunGame();
	} else if (argsCount < 4) {
		ExitMissingArgs(argsCount, args);
		return 1;
	} else {
		String_Copy(&Game_Username, &args[0]);
		String_Copy(&Game_Mppass,   &args[1]);
		String_Copy(&Server.IP,     &args[2]);

		if (!Convert_ParseUInt16(&args[3], &port)) {
			ExitInvalidArg("Invalid port", &args[3]);
			return 1;
		}
		Server.Port = port;
		Program_RunGame();
	}

	Process_Exit(0);
	return 0;
}
