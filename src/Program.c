#include "ErrorHandler.h"
#include "Platform.h"
#include "Window.h"
#include "Constants.h"
#include "Game.h"
#include "Funcs.h"
#include "ExtMath.h"
#include "Utils.h"
#include "Drawer2D.h"

/*#define CC_TEST_VORBIS*/
#ifdef CC_TEST_VORBIS
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

int main(int argc, char** argv) {
	static String defPath = String_FromConst("texpacks/default.zip");
	String title; char titleBuffer[STRING_SIZE];
	String args[PROGRAM_MAX_CMDARGS];
	int argsCount;

	uint8_t ip[4];
	uint16_t port;
	struct DisplayDevice device;
	int width, height;

	argsCount = Platform_GetCommandLineArgs(argc, argv, args);
	/* NOTE: Make sure to comment this out before pushing a commit */
	/* String rawArgs = String_FromConst("UnknownShadow200 fffff 127.0.0.1 25565"); */
	/* argsCount = String_UNSAFE_Split(&rawArgs, ' ', args, 4); */

	Platform_SetWorkingDir();
	ErrorHandler_Init();
	Platform_Init();
#ifdef CC_TEST_VORBIS
	main_imdct();
#endif
	Platform_LogConst("Starting " PROGRAM_APP_NAME " ..");

	Utils_EnsureDirectory("maps");
	Utils_EnsureDirectory("texpacks");
	Utils_EnsureDirectory("texturecache");

	if (!File_Exists(&defPath)) {
		Window_ShowDialog("Missing file",
			"default.zip is missing, try running launcher first.\n\nThe game will still run, but without any textures");
	}

	if (argsCount == 0) {
		String_AppendConst(&Game_Username, "Singleplayer");
	} else if (argsCount == 1) {
		String_Copy(&Game_Username, &args[0]);
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
			Window_ShowDialog("Failed to start", "Invalid IP");
			Platform_Exit(1); return 1;
		}
		if (!Convert_TryParseUInt16(&args[3], &port)) {
			Window_ShowDialog("Failed to start", "Invalid port");
			Platform_Exit(1); return 1;
		}
		Game_Port = port;
	}

	Options_Load();
	device = DisplayDevice_Default;
	width  = Options_GetInt(OPT_WINDOW_WIDTH,  0, device.Bounds.Width,  0);
	height = Options_GetInt(OPT_WINDOW_HEIGHT, 0, device.Bounds.Height, 0);

	/* No custom resolution has been set */
	if (width == 0 || height == 0) {
		width = 854; height = 480;
		if (device.Bounds.Width < 854) width = 640;
	}

	String_InitArray(title, titleBuffer);
	String_Format2(&title, "%c (%s)", PROGRAM_APP_NAME, &Game_Username);
	Game_Run(width, height, &title, &device);

	Platform_Exit(0);
	return 0;
}
