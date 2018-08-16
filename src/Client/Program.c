#include "Typedefs.h"
#include "ErrorHandler.h"
#include "Platform.h"
#include "Window.h"
#include "GraphicsAPI.h"
#include "Deflate.h"
#include "Formats.h"
#include "TexturePack.h"
#include "Bitmap.h"
#include "Constants.h"
#include "Game.h"
#include "Funcs.h"
#include "AsyncDownloader.h"
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

int main(void) {
	ErrorHandler_Init("client.log");
	Platform_Init();
#ifdef CC_TEST_VORBIS
	main_imdct();
#endif

	/*Http_Init();
	AsyncRequest req = { 0 };
	String url = String_FromEmptyArray(req.URL);
	String_AppendConst(&url, "http://static.classicube.net/skins/UnknownShadow200.png");
	void* reqHandle = NULL;
	ReturnCode ret = Http_MakeRequest(&req, &reqHandle);
	ReturnCode ret2 = Http_FreeRequest(reqHandle);
	ReturnCode ret3 = Http_Free();*/

	Utils_EnsureDirectory("maps");
	Utils_EnsureDirectory("texpacks");
	Utils_EnsureDirectory("texturecache");

	UChar defPathBuffer[String_BufferSize(STRING_SIZE)];
	String defPath = String_InitAndClearArray(defPathBuffer);
	String_Format1(&defPath, "texpacks%rdefault.zip", &Directory_Separator);

	if (!File_Exists(&defPath)) {
		ErrorHandler_ShowDialog("Missing file", "default.zip missing, try running launcher first");
		Platform_Exit(1);
		return 1;
	}

	Platform_LogConst("Starting " PROGRAM_APP_NAME " ..");
	Options_Init();
	Options_Load();

	struct DisplayDevice device = DisplayDevice_Default;
	Int32 width  = Options_GetInt(OPT_WINDOW_WIDTH,  0, device.Bounds.Width,  0);
	Int32 height = Options_GetInt(OPT_WINDOW_HEIGHT, 0, device.Bounds.Height, 0);

	/* No custom resolution has been set */
	if (width == 0 || height == 0) {
		width = 854; height = 480;
		if (device.Bounds.Width < 854) width = 640;
	}

	String title   = String_FromConst(PROGRAM_APP_NAME);
	String rawArgs = Platform_GetCommandLineArgs();
	/* NOTE: Make sure to comment this out before pushing a commit */
	rawArgs = String_FromReadonly("UnknownShadow200 fff 127.0.0.1 25565");

	String args[5]; Int32 argsCount = Array_Elems(args);
	String_UNSAFE_Split(&rawArgs, ' ', args, &argsCount);

	if (argsCount == 1) {
		String name = args[0];
		if (!name.length) name = String_FromReadonly("Singleplayer");
		String_Set(&Game_Username, &name);
	} else if (argsCount < 4) {
		Platform_LogConst("ClassiCube.exe is only the raw client. You must either use the launcher or provide command line arguments to start the client.");
		return;
	} else {
		String_Set(&Game_Username,  &args[0]);
		String_Set(&Game_Mppass,    &args[1]);
		String_Set(&Game_IPAddress, &args[2]);

		String bits[4]; UInt32 bitsCount = Array_Elems(bits);
		String_UNSAFE_Split(&args[2], '.', bits, &bitsCount);
		if (bitsCount != Array_Elems(bits)) {
			Platform_LogConst("Invalid IP"); return;
		}

		UInt8 ipTmp;
		if (!Convert_TryParseUInt8(&bits[0], &ipTmp) || !Convert_TryParseUInt8(&bits[1], &ipTmp) ||
			!Convert_TryParseUInt8(&bits[2], &ipTmp) || !Convert_TryParseUInt8(&bits[3], &ipTmp)) {
			Platform_LogConst("Invalid IP"); return;
		}
		
		UInt16 portTmp;
		if (!Convert_TryParseUInt16(&args[3], &portTmp)) {
			Platform_LogConst("Invalid port"); return;
		}
		Game_Port = portTmp;
	}

	Game_Run(width, height, &title, &device);
	Platform_Exit(0);
	return 0;
}

int main_test(int argc, char* argv[]) {
	return 0;
	/*
	/*BITMAPFILEHEADER bmpfileheader = { 0 };
	BITMAPV5HEADER bmpinfoheader = { 0 };
	bmpfileheader.bfType = (Int16)(('B' << 0) | ('M' << 8));
	bmpfileheader.bfOffBits = sizeof(bmpfileheader) + sizeof(bmpinfoheader);
	bmpfileheader.bfSize = Bitmap_DataSize(bmp.Width, bmp.Height) + bmpfileheader.bfOffBits;

	bmpinfoheader.bV5Size = sizeof(bmpinfoheader);
	bmpinfoheader.bV5BitCount = 32;
	bmpinfoheader.bV5Planes = 1;
	bmpinfoheader.bV5Width = bmp.Width;
	bmpinfoheader.bV5Height = -bmp.Height;
	bmpinfoheader.bV5Compression = BI_BITFIELDS;
	bmpinfoheader.bV5AlphaMask = 0xFF000000UL;
	bmpinfoheader.bV5RedMask   = 0x00FF0000UL;
	bmpinfoheader.bV5GreenMask = 0x0000FF00UL;
	bmpinfoheader.bV5BlueMask  = 0x000000FFUL; */
}
