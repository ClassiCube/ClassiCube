#include "Typedefs.h"
#include "ErrorHandler.h"
#include "Platform.h"
#include "Window.h"
#include "GraphicsAPI.h"

int main(int argc, char* argv[]) {
	ErrorHandler_Init();
	Platform_Init();

	String str = String_FromConstant("TEST");
	Window_Create(320, 320, 640, 480, &str, NULL);
	Window_SetVisible(true);
	Gfx_Init();
	Gfx_ClearColour(PackedCol_Red);

	while (true) {
		Window_ProcessEvents();
		Platform_ThreadSleep(100);
		Gfx_BeginFrame();
		Gfx_Clear();
		Gfx_EndFrame();
	}
	Gfx_Free();
	return 0;
}