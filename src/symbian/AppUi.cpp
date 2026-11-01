#include "Window_Symbian.h"

void CCAppUi::ConstructL() {
#ifdef CC_BUILD_SYMBIAN_GENERIC
	// uiq3 needs initialization of qikon things, otherwise it will crash inside BaseConstructL
	RLibrary lib;
	if (lib.Load(_L("Skins.dll")) == KErrNone) {
		TLibraryFunction func = lib.Lookup(7); // _ZN11SkinManager7CreateLER9CEikonEnv
		((void (*)(void*)) func)(CEikonEnv::Static());
		lib.Close();
	}
	if (lib.Load(_L("QikCore.dll")) == KErrNone) {
		TLibraryFunction func = lib.Lookup(306); // _ZN18CQikCommandManager7CreateLER18MQikCommandHandlerR9CEikonEnv
		((void (*)(void*, void*)) func)(NULL, CEikonEnv::Static());
		lib.Close();
	}
	p = new PImpl_();
	
	BaseConstructL(ENoAppResourceFile);
#else
	BaseConstructL(CAknAppUi::EAknEnableSkin);
	SetKeyBlockMode(ENoKeyBlock);
#endif
	
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
#ifndef CC_BUILD_SYMBIAN_GENERIC
	case EAknSoftkeyBack:
#endif
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
