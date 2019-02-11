#include "Logger.h"
#include "Platform.h"
#include "Window.h"
#include "Constants.h"
#include "Game.h"
#include "Funcs.h"
#include "Utils.h"
#include "Launcher.h"

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
	Random_Init(&rng, 2342334);
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
	struct DisplayDevice device;
	int width, height;

	if (!File_Exists(&defPath)) {
		Window_ShowDialog("Missing file",
			"default.zip is missing, try running launcher first.\n\nThe game will still run, but without any textures");
	}

	device = DisplayDevice_Default;
	width  = Options_GetInt(OPT_WINDOW_WIDTH,  0, device.Bounds.Width,  0);
	height = Options_GetInt(OPT_WINDOW_HEIGHT, 0, device.Bounds.Height, 0);

	/* No custom resolution has been set */
	if (width == 0 || height == 0) {
		width = 854; height = 480;
		if (device.Bounds.Width < 854) width = 640;
	}

	String_InitArray(title, titleBuffer);
	String_Format2(&title, "%c (%s)", GAME_APP_NAME, &Game_Username);
	Game_Run(width, height, &title);
}

static void Program_SetCurrentDirectory(void) {
	String path; char pathBuffer[FILENAME_SIZE];
	int i;
	ReturnCode res;
	String_InitArray(path, pathBuffer);

	res = Platform_GetExePath(&path);
	if (res) { Logger_Warn(res, "getting exe path"); return; }

	/* get rid of filename at end of directory */
	for (i = path.length - 1; i >= 0; i--, path.length--) {
		if (path.buffer[i] == '/' || path.buffer[i] == '\\') break;
	}
	res = Platform_SetCurrentDirectory(&path);
	if (res) { Logger_Warn(res, "setting current directory"); return; }
}

static void Program_FailInvalidArg(const char* name, const String* arg) {
	char tmpBuffer[256];
	String tmp = String_NT_Array(tmpBuffer);

	String_Format2(&tmp, "%c '%s'", name, arg);
	tmp.buffer[tmp.length] = '\0';
	Window_ShowDialog("Failed to start", tmpBuffer);
	Platform_Exit(1);
}

int main(int argc, char** argv) {
	String args[GAME_MAX_CMDARGS];
	int argsCount;
	uint8_t ip[4];
	uint16_t port;

	Logger_Hook();
	Platform_Init();
	Program_SetCurrentDirectory();
#ifdef CC_TEST_VORBIS
	main_imdct();
#endif
	Platform_LogConst("Starting " GAME_APP_NAME " ..");

	Utils_EnsureDirectory("maps");
	Utils_EnsureDirectory("texpacks");
	Utils_EnsureDirectory("texturecache");
	Utils_EnsureDirectory("plugins");
	Options_Load();

	argsCount = Platform_GetCommandLineArgs(argc, argv, args);
	/* NOTE: Make sure to comment this out before pushing a commit */
	String rawArgs = String_FromConst("UnknownShadow200 fffff 127.0.0.1 25565");
	/* String rawArgs = String_FromConst("UnknownShadow200"); */
	argsCount = String_UNSAFE_Split(&rawArgs, ' ', args, 4);

	if (argsCount == 0) {
		Launcher_Run();
	} else if (argsCount == 1) {
		String_Copy(&Game_Username, &args[0]);
		Program_RunGame();
	} else if (argsCount < 4) {
		Window_ShowDialog("Failed to start", "ClassiCube.exe is only the raw client.\n\n" \
			"Use the launcher instead, or provide command line arguments");
		Platform_Exit(1);
		return 1;
	} else {
		String_Copy(&Game_Username,  &args[0]);
		String_Copy(&Game_Mppass,    &args[1]);
		String_Copy(&Game_IPAddress, &args[2]);

		if (!Utils_ParseIP(&args[2], ip)) {
			Program_FailInvalidArg("Invalid IP", &args[2]);
			return 1;
		}
		if (!Convert_ParseUInt16(&args[3], &port)) {
			Program_FailInvalidArg("Invalid port", &args[3]);
			return 1;
		}
		Game_Port = port;
		Program_RunGame();
	}

	Platform_Exit(0);
	return 0;
}
