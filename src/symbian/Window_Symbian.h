#include <include/bitmap.h>
#include <e32base.h>
#include <e32keys.h>
#include <w32std.h>
#include <apgwgnam.h>
#include <apgcli.h>
#include <apparc.h>
#include <apgtask.h>
#include <coemain.h>
#include <coecntrl.h>
#include <eikenv.h>
#include <eikdef.h>

#include <aknutils.h>
#include <aknnotewrappers.h>
#include <aknapp.h>
#include <coecntrl.h>
#include <akndef.h>
#include <akndoc.h>
#include <aknappui.h>
#include <avkon.hrh>
#include <aknmessagequerydialog.h>
#include <aknwseventobserver.h>
#define FRAMEWORK_CLASS(name) CAkn ## name

#include <eikstart.h>
#include <baclipb.h>
#include <s32ucmp.h>
#include <classicube.rsg>
#include <e32property.h>
#include <hal.h>
#include <featdiscovery.h>
#ifndef KFeatureIdQwertyInput
#include <featureinfo.h>
#endif
extern "C" {
#include <stdlib.h>
#include <string.h>
#if CC_GFX_BACKEND == CC_GFX_BACKEND_GL1
#include <gles/egl.h>
#elif CC_GFX_BACKEND == CC_GFX_BACKEND_GL2
#include <egl/egl.h>
#endif
#include "Errors.h"
#include "Logger.h"
#include "String_.h"
#include "Gui.h"
#include "Graphics.h"
#include "Game.h"
#include "Platform.h"
#include "Options.h"
#include "Server.h"
#include "LScreens.h"
#include "Http.h"
#include "Event.h"
#include "Input.h"
#include "Window.h"
#include "Platform.h"
}

class CCContainer;

extern bool crashed;
extern CCContainer* container;

// Event management
enum CCEventType {
	CC_NONE,
	CC_KEY_DOWN, CC_KEY_UP, CC_KEY_INPUT,
	CC_TOUCH_ADD, CC_TOUCH_REMOVE
};
struct CCEvent {
	int type;
	int i1, i2, i3;
};

#define EVENTS_DEFAULT_MAX 30

class CCApp: public FRAMEWORK_CLASS(Application) {
private:
	CApaDocument* CreateDocumentL();
	TUid AppDllUid() const;
};

class CCAppUi: public FRAMEWORK_CLASS(AppUi) {
public:
	void ConstructL();
	virtual ~CCAppUi();
	void Exit() {
		CEikAppUi::Exit();
	}

private:
	void DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane);
	void HandleCommandL(TInt aCommand);
	virtual TKeyResponse HandleKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);
	virtual void HandleForegroundEventL(TBool aForeground);

private:
	CCContainer* iAppContainer;
};

class CCContainer: public CCoeControl, MCoeControlObserver, MAknWsEventObserver
{
public:
	void ConstructL(const TRect& aRect, CCAppUi* aAppUi);
	virtual ~CCContainer();
	
	void DrawFramebuffer(Rect2D r, struct Bitmap* bmp);
	static TInt LoopCallBack(TAny* aInstance);
	void RestartTimerL(TInt aInterval);
	TKeyResponse DoHandleKeyEventL(const TKeyEvent* aKeyEvent, TInt aType);

private:
	void SizeChanged();
	void HandleResourceChange(TInt aType);
	TInt CountComponentControls() const;
	CCoeControl* ComponentControl(TInt aIndex) const;
	void Draw(const TRect& aRect) const;
	virtual void HandleControlEventL(CCoeControl*, TCoeEvent);
	virtual void HandlePointerEventL(const TPointerEvent& aPointerEvent);
	virtual void HandleWsEventL(const TWsEvent &aEvent, CCoeControl *aDestination);
	
public:
	void InitEvents();
	void PushEvent(const CCEvent* event);
	cc_bool PullEvent(CCEvent* event);

public:
	CCAppUi*  iAppUi;
	CFbsBitmap* iBitmap;
	CPeriodic*  iPeriodic;
	
	bool gameRunning;
	bool useAknWsEventMonitor;
	void* events_mutex;
	int events_count, events_capacity;
	CCEvent* events_list, events_default[EVENTS_DEFAULT_MAX];
};

class CCDocument: public FRAMEWORK_CLASS(Document) {
public:
	static CCDocument* NewL(CEikApplication& aApp);
	virtual ~CCDocument();

protected:
	void ConstructL();

public:
	CCDocument(CEikApplication& aApp);

private:
	CEikAppUi* CreateAppUiL();
};
