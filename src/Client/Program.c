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
#include "Audio.h"
#include "ExtMath.h"

#ifdef CC_TEST_VORBIS
#include <Windows.h>
#pragma comment(lib, "Winmm.lib")

int main_imdct() {
	Real32 in[32], out[64], out2[64];
	Random rng;
	Random_Init(&rng, 2342334);
	for (int ii = 0; ii < 32; ii++) {
		in[ii] = Random_Float(&rng);
	}

	imdct(in, out, 32);
	imdct_fast(in, out2, 64);
	int fff = 0;
}

void vorbis_test() {
	void* file;
	String oggPath = String_FromConst("audio/calm1.ogg");
	File_Open(&file, &oggPath);
	struct Stream oggBase;
	Stream_FromFile(&oggBase, file, &oggPath);
	struct Stream ogg;
	UInt8 oggBuffer[OGG_BUFFER_SIZE];
	Ogg_MakeStream(&ogg, oggBuffer, &oggBase);

	struct VorbisState state;
	Vorbis_Init(&state, &ogg);
	Vorbis_DecodeHeaders(&state);
	Int32 size = 0, offset = 0;

	Int16* chanData = Mem_Alloc(state.Channels * state.SampleRate * 1000, sizeof(Int16), "tmp data");
	for (int i = 0; i < 1000; i++) {
		Vorbis_DecodeFrame(&state);
		Int32 read = Vorbis_OutputFrame(&state, &chanData[offset]);

		size += read;
		offset += read * state.Channels;
	}

	WAVEFORMATEX format = { 0 };
	format.nChannels = state.Channels;
	format.nSamplesPerSec = state.SampleRate;
	format.wBitsPerSample = 16;
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
	format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

	unsigned devices = waveOutGetNumDevs();
	HWAVEOUT handle;
	unsigned res1 = waveOutOpen(&handle, UINT_MAX, &format, 0, 0, 0);

	WAVEHDR header = { 0 };
	header.lpData = chanData;
	header.dwBufferLength = offset * 2;

	unsigned res2 = waveOutPrepareHeader(handle, &header, sizeof(header));
	unsigned res3 = waveOutWrite(handle, &header, sizeof(header));
	Thread_Sleep(20000);
	unsigned res4 = res3;
}
#endif

int main(void) {
	ErrorHandler_Init("client.log");
	Platform_Init();
#ifdef CC_TEST_VORBIS
	//main_imdct();
	vorbis_test();
#endif

	/*Http_Init();
	AsyncRequest req = { 0 };
	String url = String_FromEmptyArray(req.URL);
	String_AppendConst(&url, "http://static.classicube.net/skins/UnknownShadow200.png");
	void* reqHandle = NULL;
	ReturnCode ret = Http_MakeRequest(&req, &reqHandle);
	ReturnCode ret2 = Http_FreeRequest(reqHandle);
	ReturnCode ret3 = Http_Free();*/

	String maps = String_FromConst("maps");
	if (!Directory_Exists(&maps)) {
		ReturnCode result = Directory_Create(&maps);
		ErrorHandler_CheckOrFail(result, "Program - creating maps directory");
	}

	String texPacks = String_FromConst("texpacks");
	if (!Directory_Exists(&texPacks)) {
		ReturnCode result = Directory_Create(&texPacks);
		ErrorHandler_CheckOrFail(result, "Program - creating texpacks directory");
	}

	String texCache = String_FromConst("texturecache");
	if (!Directory_Exists(&texCache)) {
		ReturnCode result = Directory_Create(&texCache);
		ErrorHandler_CheckOrFail(result, "Program - creating texturecache directory");
	}

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
	//rawArgs = String_FromReadonly("UnknownShadow200 fff 127.0.0.1 25566");

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
