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

int main(int argc, char* argv[]) {
	ErrorHandler_Init("client.log");
	Platform_Init();

	String maps = String_FromConst("maps");
	if (!Platform_DirectoryExists(&maps)) {
		ReturnCode result = Platform_DirectoryCreate(&maps);
		ErrorHandler_CheckOrFail(result, "Program - creating maps directory");
	}

	String texPacks = String_FromConst("texpacks");
	if (!Platform_DirectoryExists(&texPacks)) {
		ReturnCode result = Platform_DirectoryCreate(&texPacks);
		ErrorHandler_CheckOrFail(result, "Program - creating texpacks directory");
	}

	String texCache = String_FromConst("texturecache");
	if (!Platform_DirectoryExists(&texCache)) {
		ReturnCode result = Platform_DirectoryCreate(&texCache);
		ErrorHandler_CheckOrFail(result, "Program - creating texturecache directory");
	}

	Platform_LogConst("Starting " PROGRAM_APP_NAME " ..");
	Options_Init();
	Options_Load();

	DisplayDevice device = DisplayDevice_Default;
	Int32 width  = Options_GetInt(OPT_WINDOW_WIDTH,  0, device.Bounds.Width,  0);
	Int32 height = Options_GetInt(OPT_WINDOW_HEIGHT, 0, device.Bounds.Height, 0);

	/* No custom resolution has been set */
	if (width == 0 || height == 0) {
		width = 854; height = 480;
		if (device.Bounds.Width < 854) width = 640;
	}

	String title = String_FromConst(PROGRAM_APP_NAME);
	//argc = 5;
	//char* default_argv[5] = { "path", "UnknownShadow200", "fff", "127.0.0.1", "25566" };
	argc = 2;
	char* default_argv[2] = { "path", "UnknownShadow200" };
	argv = default_argv;

	if (argc == 1 || argc == 2) {
		String_AppendConst(&Game_Username, argc > 1 ? argv[1] : "Singleplayer");
		//String_AppendConst(&Game_Username, "Singleplayer");
	} else if (argc < 5) {
		Platform_LogConst("ClassicalSharp.exe is only the raw client. You must either use the launcher or provide command line arguments to start the client.");
		return;
	} else {
		String_AppendConst(&Game_Username, argv[1]);
		String_AppendConst(&Game_Mppass,   argv[2]);
		String ipRaw   = String_FromReadonly(argv[3]);
		String portRaw = String_FromReadonly(argv[4]);

		String bits[4]; UInt32 bitsCount = 4;
		String_UNSAFE_Split(&ipRaw, '.', bits, &bitsCount);
		if (bitsCount != 4) {
			Platform_LogConst("Invalid IP"); return;
		}

		UInt8 ipTmp;
		if (!Convert_TryParseUInt8(&bits[0], &ipTmp) || !Convert_TryParseUInt8(&bits[1], &ipTmp) ||
			!Convert_TryParseUInt8(&bits[2], &ipTmp) || !Convert_TryParseUInt8(&bits[3], &ipTmp)) {
			Platform_LogConst("Invalid IP"); return;
		}
		
		UInt16 portTmp;
		if (!Convert_TryParseUInt16(&portRaw, &portTmp)) {
			Platform_LogConst("Invalid port"); return;
		}

		String_Set(&Game_IPAddress, &ipRaw);
		Game_Port = portTmp;
	}

	Game_Run(width, height, &title, &device);
	Platform_Exit(0);
	return 0;
}

/*
String text1 = String_FromConst("abcd");
String lines1[3] = { 0 };
WordWrap_Do(&text1, lines1, 3, 4);

String text2 = String_FromConst("abcde/fgh");
String lines2[3] = { 0 };
WordWrap_Do(&text2, lines2, 3, 4);

String text3 = String_FromConst("abc/defg");
String lines3[3] = { 0 };
WordWrap_Do(&text3, lines3, 3, 4);

String text4 = String_FromConst("ab/cdef");
String lines4[3] = { 0 };
WordWrap_Do(&text4, lines4, 3, 4);

String text5 = String_FromConst("abcd/efg");
String lines5[3] = { 0 };
WordWrap_Do(&text5, lines5, 3, 4);

String text6 = String_FromConst("abc/efg/hij/");
String lines6[3] = { 0 };
WordWrap_Do(&text6, lines6, 3, 4);

String text7 = String_FromConst("ab cde fgh");
String lines7[3] = { 0 };
WordWrap_Do(&text7, lines7, 3, 4);

String text8 = String_FromConst("ab//cd");
String lines8[3] = { 0 };
WordWrap_Do(&text8, lines8, 3, 4);

String text9 = String_FromConst("a///b");
String lines9[3] = { 0 };
WordWrap_Do(&text9, lines9, 3, 4);

String text10 = String_FromConst("/aaab");
String lines10[3] = { 0 };
WordWrap_Do(&text10, lines10, 3, 4);
*/

//#include <Windows.h>
int main_test(int argc, char* argv[]) {
	return 0;
	/*void* file;
	String path = String_FromConstant("H:\\PortableApps\\GitPortable\\App\\Git\\ClassicalSharp\\output\\release\\texpacks\\skybox.png");
	ReturnCode openCode = Platform_FileOpen(&file, &path);
	Stream fileStream;
	Stream_FromFile(&fileStream, file, &path);
	Bitmap bmp;
	Bitmap_DecodePng(&bmp, &fileStream);*/

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
	bmpinfoheader.bV5BlueMask  = 0x000000FFUL;

	void* file2;
	String path2 = String_FromConstant("H:\\PortableApps\\GitPortable\\App\\Git\\ClassicalSharp\\output\\release\\texpacks\\skybox8.bmp");
	openCode = Platform_FileCreate(&file2, &path2);
	Stream fileStream2;
	Stream_FromFile(&fileStream2, file2, &path2);
	Stream_Write(&fileStream2, &bmpfileheader, sizeof(bmpfileheader));
	Stream_Write(&fileStream2, &bmpinfoheader, sizeof(bmpinfoheader));
	Stream_Write(&fileStream2, bmp.Scan0, Bitmap_DataSize(bmp.Width, bmp.Height));
	fileStream2.Close(&fileStream2);*/

	//return 0;

	/*void* file;
	String path = String_FromConstant("H:\\PortableApps\\GitPortable\\App\\Git\\ClassicalSharp\\output\\release\\texpacks\\default.zip");
	ReturnCode openCode = Platform_FileOpen(&file, &path);
	Stream fileStream;
	Stream_FromFile(&fileStream, file, &path);
	ZipState state;
	Zip_Init(&state, &fileStream);
	Zip_Extract(&state);
	return 0;*/

	/*void* file;
	String path = String_FromConst("H:\\PortableApps\\GitPortable\\App\\Git\\ClassicalSharp\\src\\x64\\Release\\canyon.lvl");
	ReturnCode openCode = Platform_FileOpen(&file, &path);
	Stream fileStream;
	Stream_FromFile(&fileStream, file, &path);
	Lvl_Load(&fileStream);
	return 0;*/

	/*void* file;
	String path = String_FromConstant("H:\\PortableApps\\GitPortable\\App\\Git\\\ClassicalSharp\\src\\Debug\\gunzip.c.gz");
	ReturnCode openCode = Platform_FileOpen(&file, &path);
	Stream fileStream;
	Stream_FromFile(&fileStream, file, &path);

	GZipHeader gzip;
	GZipHeader_Init(&gzip);
	while (!gzip.Done) { GZipHeader_Read(&fileStream, &gzip); }
	UInt32 pos = Platform_FilePosition(file);

	InflateState deflate;
	Inflate_Init(&deflate, &fileStream);
	UInt32 read;
	fileStream.Read(&fileStream, deflate.Input, 8192, &read);
	deflate.AvailIn = read;

	UInt8 out[56000];
	deflate.Output = out;
	deflate.AvailOut = 56000;
	//puff(out, &deflate.AvailOut, deflate.Input, &deflate.AvailIn);
	Inflate_Process(&deflate);

	String path2 = String_FromConstant("H:\\PortableApps\\GitPortable\\App\\Git\\ClassicalSharp\\src\\x64\\Debug\\ffff.c");
	openCode = Platform_FileCreate(&file, &path2);
	Stream_FromFile(&fileStream, file, &path);
	UInt32 written;
	fileStream.Write(&fileStream, out, 56000 - deflate.AvailOut, &written);
	fileStream.Close(&fileStream);


	return;
	String str = String_FromConstant("TEST");
	Window_Create(320, 320, 640, 480, &str, NULL);
	Window_SetVisible(true);
	Gfx_Init();

	UInt8 RGB = 0;
	while (true) {
		Gfx_ClearColour(PackedCol_Create3(RGB, RGB, RGB));
		RGB++;
		Window_ProcessEvents();
		Platform_ThreadSleep(100);
		Gfx_BeginFrame();
		Gfx_Clear();
		Gfx_EndFrame();
	}
	Gfx_Free();
	return 0;*/
}