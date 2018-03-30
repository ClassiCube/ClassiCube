#include "Typedefs.h"
#include "ErrorHandler.h"
#include "Platform.h"
#include "Window.h"
#include "GraphicsAPI.h"
#include "Deflate.h"
#include "Formats.h"
#include "ZipArchive.h"
#include "Bitmap.h"
#include <Windows.h>

int main(int argc, char* argv[]) {
	ErrorHandler_Init();
	Platform_Init();

	String maps = String_FromConst("maps");
	if (!Platform_DirectoryExists(&maps)) {
		Platform_DirectoryCreate(&maps);
	}
	String texPacks = String_FromConst("texpacks");
	if (!Platform_DirectoryExists(&texPacks)) {
		Platform_DirectoryCreate(&texPacks);
	}

	/*void* file;
	String path = String_FromConstant("H:\\PortableApps\\GitPortable\\App\\Git\\ClassicalSharp\\output\\release\\texpacks\\skybox.png");
	ReturnCode openCode = Platform_FileOpen(&file, &path, true);
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
	ReturnCode openCode = Platform_FileOpen(&file, &path, true);
	Stream fileStream;
	Stream_FromFile(&fileStream, file, &path);
	ZipState state;
	Zip_Init(&state, &fileStream);
	Zip_Extract(&state);
	return 0;*/

	void* file;
	String path = String_FromConst("H:\\PortableApps\\GitPortable\\App\\Git\\ClassicalSharp\\src\\x64\\Release\\canyon.lvl");
	ReturnCode openCode = Platform_FileOpen(&file, &path, true);
	Stream fileStream;
	Stream_FromFile(&fileStream, file, &path);
	Lvl_Load(&fileStream);
	return 0;

	/*void* file;
	String path = String_FromConstant("H:\\PortableApps\\GitPortable\\App\\Git\\\ClassicalSharp\\src\\Debug\\gunzip.c.gz");
	ReturnCode openCode = Platform_FileOpen(&file, &path, true);
	Stream fileStream;
	Stream_FromFile(&fileStream, file, &path);

	GZipHeader gzip;
	GZipHeader_Init(&gzip);
	while (!gzip.Done) { GZipHeader_Read(&fileStream, &gzip); }
	UInt32 pos = Platform_FilePosition(file);

	DeflateState deflate;
	Deflate_Init(&deflate, &fileStream);
	UInt32 read;
	fileStream.Read(&fileStream, deflate.Input, 8192, &read);
	deflate.AvailIn = read;

	UInt8 out[56000];
	deflate.Output = out;
	deflate.AvailOut = 56000;
	//puff(out, &deflate.AvailOut, deflate.Input, &deflate.AvailIn);
	Deflate_Process(&deflate);

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