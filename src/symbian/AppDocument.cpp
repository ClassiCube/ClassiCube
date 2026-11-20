#include "Window_Symbian.h"

CCDocument::CCDocument(CEikApplication& aApp) :
	FRAMEWORK_CLASS(Document)(aApp) {
}

CCDocument::~CCDocument() {
}

void CCDocument::ConstructL() {
}

CCDocument* CCDocument::NewL(CEikApplication& aApp) {
	CCDocument* self = new (ELeave) CCDocument(aApp);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();

	return self;
}

CEikAppUi* CCDocument::CreateAppUiL() {
	return new (ELeave) CCAppUi;
}
