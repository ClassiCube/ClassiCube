#include "ErrorHandler.h"
#if CC_BUILD_NIX
#include "Platform.h"

void ErrorHandler_Init(const UChar* logFile) {
	/* TODO: Implement this */
}

void ErrorHandler_Log(STRING_PURE String* msg) {
	/* TODO: Implement this */
}

void ErrorHandler_Fail(const UChar* raw_msg) {
	/* TODO: Implement this */
	Platform_Exit(1);
}

void ErrorHandler_FailWithCode(ReturnCode code, const UChar* raw_msg) {
	/* TODO: Implement this */
	Platform_Exit(code);
}

void ErrorHandler_ShowDialog(const UChar* title, const UChar* msg) {
	/* TODO: Implement this */
}

#endif
