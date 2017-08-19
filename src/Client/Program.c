#include "Typedefs.h"
#include "ErrorHandler.h"
#include "Platform.h"
#include "Window.h"
#include "GraphicsAPI.h"

int main(int argc, char* argv[]) {
	ErrorHandler_Init();
	Platform_Init();

	String str = String_FromConstant("TEST");
	Window_Create(320, 320, 320, 320, &str, NULL);
	Window_SetVisible(true);
	Gfx_Init();
	while (true) {
		Window_ProcessEvents();
		Platform_ThreadSleep(100);
	}
	return 0;
}