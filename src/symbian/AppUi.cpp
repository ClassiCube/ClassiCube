#include "Window_Symbian.h"

void CCAppUi::ConstructL() {
	BaseConstructL(CAknAppUi::EAknEnableSkin);
	SetKeyBlockMode(ENoKeyBlock);
	
	iAppContainer = new (ELeave) CCContainer;
	iAppContainer->SetMopParent(this);
	iAppContainer->ConstructL(ClientRect(), this);
	AddToStackL(iAppContainer);
}

CCAppUi::~CCAppUi() {
	if (iAppContainer) {
		RemoveFromStack(iAppContainer);
		delete iAppContainer;
	}
}

void CCAppUi::DynInitMenuPaneL(TInt, CEikMenuPane*) { }

void CCAppUi::HandleForegroundEventL(TBool aForeground) {
	WindowInfo.Inactive = !aForeground;
	Event_RaiseVoid(&WindowEvents.InactiveChanged);
}

TKeyResponse CCAppUi::HandleKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType) {
	if (iAppContainer->useAknWsEventMonitor) {
		return EKeyWasNotConsumed;
	}
	
	return iAppContainer->DoHandleKeyEventL(&aKeyEvent, aType);
}

void CCAppUi::HandleCommandL(TInt aCommand) {	
	switch (aCommand) {
	case EAknSoftkeyBack:
	case EEikCmdExit: {
		WindowInfo.Exists = false;
		Window_RequestClose();
		Exit();
		break;
	}
	default:
		break;
	}
}
