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
	User::SetFloatingPointMode(EFpModeRunFast);
	return EikStart::RunApplication(NewApplication);
}
