#include "Typedefs.h"
#include "ErrorHandler.h"
#include "Platform.h"
#include "Window.h"

int main(int argc, char* argv[]) {
	ErrorHandler_Init();
	Platform_Init();

	Window_Create(0, 0, 320, 320, "TEST", NULL);
	Window_SetVisible(true);
	while (true) {
		Window_ProcessEvents();
		Platform_ThreadSleep(100);
	}
	return 0;
}