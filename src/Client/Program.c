#include "Typedefs.h"
#include "ErrorHandler.h"
#include "Platform.h"

int main(int argc, char* argv[]) {
	ErrorHandler_Init();
	Platform_Init();
	return 0;
}