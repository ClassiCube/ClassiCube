#include "../../src/Core.h"
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Devices::Input;
using namespace Windows::Graphics::Display;
using namespace Windows::Foundation;
using namespace Windows::System;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Platform;

#include "../../src/_WindowBase.h"
#include "../../src/String.h"
#include "../../src/Funcs.h"
#include "../../src/Bitmap.h"
#include "../../src/Options.h"
#include "../../src/Errors.h"


/*########################################################################################################################*
*--------------------------------------------------Public implementation--------------------------------------------------*
*#########################################################################################################################*/
void Window_PreInit(void) { 
	DisplayInfo.CursorVisible = true;
}

void Window_Init(void) {
	Input.Sources = INPUT_SOURCE_NORMAL;

	DisplayInfo.Width  = 640;
	DisplayInfo.Height = 480;
	DisplayInfo.Depth  = 32;
	DisplayInfo.ScaleX = 1.0f;
	DisplayInfo.ScaleY = 1.0f;
}

void Window_Free(void) { }

void Window_Create2D(int width, int height) { }
void Window_Create3D(int width, int height) { }

void Window_Destroy(void) {
}

void Window_SetTitle(const cc_string* title) {
}

void Clipboard_GetText(cc_string* value) {
}

void Clipboard_SetText(const cc_string* value) {
}

int Window_GetWindowState(void) {
	return WINDOW_STATE_NORMAL;
}

cc_result Window_EnterFullscreen(void) {
	return ERR_NOT_SUPPORTED;
}

cc_result Window_ExitFullscreen(void) {
	return ERR_NOT_SUPPORTED;
}

int Window_IsObscured(void) { return 0; }

void Window_Show(void) {
}

void Window_SetSize(int width, int height) {
}

void Window_RequestClose(void) {
	Event_RaiseVoid(&WindowEvents.Closing);
}

void Window_ProcessEvents(float delta) {
}

void Gamepads_Init(void) {

}

void Gamepads_Process(float delta) { }

static void Cursor_GetRawPos(int* x, int* y) {
	*x = 0;
	*y = 0;
}

void Cursor_SetPosition(int x, int y) { 
}

static void Cursor_DoSetVisible(cc_bool visible) {
}

static void ShowDialogCore(const char* title, const char* msg) {
}

static cc_result OpenSaveFileDialog(const cc_string* filters, FileDialogCallback callback, cc_bool load,
									const char* const* fileExts, const cc_string* defaultName) {
	return ERR_NOT_SUPPORTED;
}

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args) {
	const char* const* fileExts = args->filters;
	cc_string filters; char buffer[NATIVE_STR_LEN];
	int i;

	/* Filter tokens are \0 separated - e.g. "Maps (*.cw;*.dat)\0*.cw;*.dat\0 */
	String_InitArray(filters, buffer);
	String_Format1(&filters, "%c (", args->description);
	for (i = 0; fileExts[i]; i++)
	{
		if (i) String_Append(&filters, ';');
		String_Format1(&filters, "*%c", fileExts[i]);
	}
	String_Append(&filters, ')');
	String_Append(&filters, '\0');

	for (i = 0; fileExts[i]; i++)
	{
		if (i) String_Append(&filters, ';');
		String_Format1(&filters, "*%c", fileExts[i]);
	}
	String_Append(&filters, '\0');

	return OpenSaveFileDialog(&filters, args->Callback, true, fileExts, &String_Empty);
}

cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args) {
	const char* const* titles   = args->titles;
	const char* const* fileExts = args->filters;
	cc_string filters; char buffer[NATIVE_STR_LEN];
	int i;

	/* Filter tokens are \0 separated - e.g. "Map (*.cw)\0*.cw\0 */
	String_InitArray(filters, buffer);
	for (i = 0; fileExts[i]; i++)
	{
		String_Format2(&filters, "%c (*%c)", titles[i], fileExts[i]);
		String_Append(&filters,  '\0');
		String_Format1(&filters, "*%c", fileExts[i]);
		String_Append(&filters,  '\0');
	}
	return OpenSaveFileDialog(&filters, args->Callback, false, fileExts, &args->defaultName);
}

void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	bmp->scan0  = (BitmapCol*)Mem_Alloc(width * height, BITMAPCOLOR_SIZE, "bitmap");
	bmp->width  = width;
	bmp->height = height;
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
}

static cc_bool rawMouseInited, rawMouseSupported;
static void InitRawMouse(void) {
	
}

void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) { }
void OnscreenKeyboard_SetText(const cc_string* text) { }
void OnscreenKeyboard_Close(void) { }

void Window_EnableRawMouse(void) {
	DefaultEnableRawMouse();
	if (!rawMouseInited) InitRawMouse();

	rawMouseInited = true;
}

void Window_UpdateRawMouse(void) {
	if (rawMouseSupported) {
		/* handled in WM_INPUT messages */
		CentreMousePosition();
	} else {
		DefaultUpdateRawMouse();
	}
}

void Window_DisableRawMouse(void) { 
	DefaultDisableRawMouse();
}


void OpenFileDialog(void) {
    auto picker = ref new Windows::Storage::Pickers::FileOpenPicker();
    picker->FileTypeFilter->Append(ref new String(L".jpg"));
    picker->FileTypeFilter->Append(ref new String(L".jpeg"));
    picker->FileTypeFilter->Append(ref new String(L".png"));

    //Windows::Storage::StorageFile file = picker->PickSingleFileAsync();
}

ref class CCApp sealed : IFrameworkView
{
public:
    virtual void Initialize(CoreApplicationView^ view)
    {
    }

    virtual void Load(String^ EntryPoint)
    {
    }

    virtual void Uninitialize()
    {
    }

    virtual void Run()
    {
        CoreWindow^ window = CoreWindow::GetForCurrentThread();
        window->Activate();

        CoreDispatcher^ dispatcher = window->Dispatcher;
        dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessUntilQuit);
    }

    virtual void SetWindow(CoreWindow^ win)
    {
    }
};

ref class CCAppSource sealed : IFrameworkViewSource
{
public:
    virtual IFrameworkView^ CreateView()
    {
        return ref new CCApp();
    }
};

[MTAThread]
int main(Array<String^>^ args)
{
    //init_apartment();
    auto source = ref new CCAppSource();
    CoreApplication::Run(source);
}