#include "Window_Symbian.h"

const TUid KUidClassiCube = {0xE212A5C2};

TUid CCApp::AppDllUid() const {
	return KUidClassiCube;
}

CApaDocument* CCApp::CreateDocumentL() {
	return CCDocument::NewL(*this);
}

LOCAL_C CApaApplication* NewApplication();

CApaApplication* NewApplication() {
	return new CCApp;
}

GLDEF_C TInt E32Main();

TInt E32Main() {
#ifndef CC_BUILD_SYMBIAN_S60V3
	User::SetFloatingPointMode(EFpModeRunFast);
#endif
	return EikStart::RunApplication(NewApplication);
}

#if !defined __ARMCC_4_0__ && defined EKA2
extern "C" {
extern int __aeabi_uidivmod(unsigned int a, unsigned int b);
extern int __aeabi_idivmod(int a, int b);
int __aeabi_idiv(int a, int b) {
	return __aeabi_idivmod(a, b);
}

int __aeabi_uidiv(unsigned int a, unsigned int b) {
	return __aeabi_uidivmod(a, b);
}
}
#endif

#ifdef CC_BUILD_SYMBIAN_S60V3
extern "C" {
// hack for 9.1, linker will use this stub instead of importing it from dll
extern int _ZN7UserSvr9DllSetTlsEiiPv(int, int, void*);
int _ZN7UserSvr9DllSetTlsEiiPv(int, int, void*) {
	return 0;
}
}
#endif
