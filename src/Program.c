#include "Core.h"
#include "ErrorHandler.h"
#include "Platform.h"
#include "Window.h"
#include "Constants.h"
#include "Game.h"
#include "Funcs.h"
#include "ExtMath.h"
#include "Utils.h"

//#define CC_TEST_VORBIS
#ifdef CC_TEST_VORBIS
#include "Vorbis.h"

#define VORBIS_N 1024
#define VORBIS_N2 (VORBIS_N / 2)
int main_imdct() {
	Real32 in[VORBIS_N2], out[VORBIS_N], out2[VORBIS_N];
	Real64 delta[VORBIS_N];

	Random rng;
	Random_Init(&rng, 2342334);
	struct imdct_state imdct;
	imdct_init(&imdct, VORBIS_N);

	for (int ii = 0; ii < VORBIS_N2; ii++) {
		in[ii] = Random_Float(&rng);
	}

	imdct_slow(in, out, VORBIS_N2);
	imdct_calc(in, out2, &imdct);

	Real64 sum = 0;
	for (int ii = 0; ii < VORBIS_N; ii++) {
		delta[ii] = out2[ii] - out[ii];
		sum += delta[ii];
	}
	int fff = 0;
}
#endif

int main(int argc, char** argv) {
	Platform_SetWorkingDir();
	ErrorHandler_Init("client.log");
	Platform_Init();
#ifdef CC_TEST_VORBIS
	main_imdct();
#endif
	Platform_LogConst("Starting " PROGRAM_APP_NAME " ..");

	Utils_EnsureDirectory("maps");
	Utils_EnsureDirectory("texpacks");
	Utils_EnsureDirectory("texturecache");

	char defPathBuffer[STRING_SIZE];
	String defPath = String_FromArray(defPathBuffer);
	String_Format1(&defPath, "texpacks%rdefault.zip", &Directory_Separator);

	if (!File_Exists(&defPath)) {
		ErrorHandler_ShowDialog("Failed to start", "default.zip is missing, try running launcher first");
		Platform_Exit(1); return 1;
	}

	String args[PROGRAM_MAX_CMDARGS];
	String title    = String_FromConst(PROGRAM_APP_NAME);
	Int32 argsCount = Platform_GetCommandLineArgs(argc, argv, args);

	/* NOTE: Make sure to comment this out before pushing a commit */
	// String rawArgs = String_FromConst("UnknownShadow200 fff 127.0.0.1 25565");
	// argsCount = 4; String_UNSAFE_Split(&rawArgs, ' ', args, &argsCount);

	if (argsCount == 1) {
		String name = args[0];
		if (!name.length) name = String_FromReadonly("Singleplayer");
		String_Copy(&Game_Username, &name);
	} else if (argsCount < 4) {
		ErrorHandler_ShowDialog("Failed to start", "ClassiCube.exe is only the raw client.\n\n" \
			"Use the launcher instead, or provide command line arguments");
		Platform_Exit(1);
		return 1;
	} else {
		String_Copy(&Game_Username,  &args[0]);
		String_Copy(&Game_Mppass,    &args[1]);
		String_Copy(&Game_IPAddress, &args[2]);

		UInt8 ip[4];
		if (!Utils_ParseIP(&args[2], ip)) { 
			ErrorHandler_ShowDialog("Failed to start", "Invalid IP");
			Platform_Exit(1); return 1; 
		}
		
		UInt16 port;
		if (!Convert_TryParseUInt16(&args[3], &port)) { 
			ErrorHandler_ShowDialog("Failed to start", "Invalid port");
			Platform_Exit(1); return 1;
		}
		Game_Port = port;
	}

	Options_Load();
	struct DisplayDevice device = DisplayDevice_Default;
	Int32 width  = Options_GetInt(OPT_WINDOW_WIDTH,  0, device.Bounds.Width,  0);
	Int32 height = Options_GetInt(OPT_WINDOW_HEIGHT, 0, device.Bounds.Height, 0);

	/* No custom resolution has been set */
	if (width == 0 || height == 0) {
		width = 854; height = 480;
		if (device.Bounds.Width < 854) width = 640;
	}

	Game_Run(width, height, &title, &device);
	Platform_Exit(0);
	return 0;
}
