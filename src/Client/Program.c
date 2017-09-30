#include "Typedefs.h"
#include "ErrorHandler.h"
#include "Platform.h"
#include "Window.h"
#include "GraphicsAPI.h"
#include "Deflate.h"

int main(int argc, char* argv[]) {
	ErrorHandler_Init();
	Platform_Init();

	void* file;
	String path = String_FromConstant("H:\\PortableApps\\GitPortable\\App\\Git\\ClassicalSharp\\src\\Debug\\gunzip.c.gz");
	ReturnCode openCode = Platform_FileOpen(&file, &path, true);
	Stream fileStream;
	Stream_FromFile(file, &fileStream, &path);
	
	GZipHeader gzip;
	GZipHeader_Init(&gzip);
	while (!gzip.Done) { GZipHeader_Read(&fileStream, &gzip); }
	UInt32 pos = Platform_FilePosition(file);

	DeflateState deflate;
	Deflate_Init(&deflate, &fileStream);
	Stream_Read(&fileStream, &deflate.Input, 1024);
	deflate.AvailIn = 1024;

	UInt8 out[560];
	deflate.Output = out;
	deflate.AvailOut = 560;
	Deflate_Process(&deflate);

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
	return 0;
}