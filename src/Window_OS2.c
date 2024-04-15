#include "Core.h"
#if defined CC_BUILD_OS2 && !defined CC_BUILD_SDL2
#include "_WindowBase.h"
#include "String.h"
#include "Funcs.h"
#include "Bitmap.h"
#include "Options.h"
#include "Errors.h"

#define INCL_DOSPROCESS
#define INCL_DOSMEMMGR
#define INCL_DOSERRORS
#define INCL_PM
#define INCL_MMIOOS2
#include <os2.h>
#include <os2me.h>
#include <dive.h>
#include <string.h>
#include <stdio.h>

#define CC_WIN_STYLE WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN
#define CC_WIN_CLASSNAME "ClassiCube_Window"
#define Rect_Width(rect)  (rect.xRight  - rect.xLeft)
#define Rect_Height(rect) (rect.yBottom - rect.yTop)

static HAB habAnchor;
static HMQ msgQueue;
static HWND hwndFrame;
static HWND hwndClient;
static HDIVE hDive = NULLHANDLE;
static ULONG bufNum = 0;
static PBYTE imageBuffer = NULL;

static HDC win_DC;
static cc_bool suppress_resize;
static int win_totalWidth, win_totalHeight; /* Size of window including titlebar and borders */
static cc_bool grabCursor;
static int windowX, windowY;

typedef struct _WINDATA {
	HWND hwndFrame;
	HWND hwnd;
	PFNWP fnUserWndProc;
	PFNWP fnWndFrameProc;
} WINDATA;

static const cc_uint8 key_map[14 * 16] = {
	0, 0, 0, 0, 0, 0, 0, 0, CCKEY_BACKSPACE, CCKEY_TAB, 0, 0, 0, CCKEY_ENTER, 0, 0,
	0, 0, 0, CCKEY_PAUSE, CCKEY_CAPSLOCK, 0, 0, 0, 0, 0, 0, CCKEY_ESCAPE, 0, 0, 0, 0,
	CCKEY_SPACE, CCKEY_PAGEUP, CCKEY_PAGEDOWN, CCKEY_END, CCKEY_HOME, CCKEY_LEFT, CCKEY_UP, CCKEY_RIGHT, CCKEY_DOWN, 0, CCKEY_PRINTSCREEN, 0, CCKEY_PRINTSCREEN, CCKEY_INSERT, CCKEY_DELETE, 0,
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 0, 0, 0, 0, 0, 0,
	0, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', CCKEY_LWIN, CCKEY_RWIN, CCKEY_MENU, 0, 0,
	CCKEY_KP0, CCKEY_KP1, CCKEY_KP2, CCKEY_KP3, CCKEY_KP4, CCKEY_KP5, CCKEY_KP6, CCKEY_KP7, CCKEY_KP8, CCKEY_KP9, CCKEY_KP_MULTIPLY, CCKEY_KP_PLUS, 0, CCKEY_KP_MINUS, CCKEY_KP_DECIMAL, CCKEY_KP_DIVIDE,
	CCKEY_F1, CCKEY_F2, CCKEY_F3, CCKEY_F4, CCKEY_F5, CCKEY_F6, CCKEY_F7, CCKEY_F8, CCKEY_F9, CCKEY_F10, CCKEY_F11, CCKEY_F12, CCKEY_F13, CCKEY_F14, CCKEY_F15, CCKEY_F16,
	CCKEY_F17, CCKEY_F18, CCKEY_F19, CCKEY_F20, CCKEY_F21, CCKEY_F22, CCKEY_F23, CCKEY_F24, 0, 0, 0, 0, 0, 0, 0, 0,
	CCKEY_NUMLOCK, CCKEY_SCROLLLOCK, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	CCKEY_LSHIFT, CCKEY_RSHIFT, CCKEY_LCTRL, CCKEY_RCTRL, CCKEY_LALT, CCKEY_RALT, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, CCKEY_SEMICOLON, CCKEY_EQUALS, CCKEY_COMMA, CCKEY_MINUS, CCKEY_PERIOD, CCKEY_SLASH,
	CCKEY_TILDE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, CCKEY_LBRACKET, CCKEY_BACKSLASH, CCKEY_RBRACKET, CCKEY_QUOTE, 0,
};

static int MapNativeKey(MPARAM mp1, MPARAM mp2) {
	USHORT flags = SHORT1FROMMP(mp1);
	USHORT scanCode = CHAR4FROMMP(mp1);
	USHORT key = SHORT2FROMMP(mp2);
	
	if (flags & KC_VIRTUALKEY) 
	{
		switch (key)
		{
		case VK_CTRL:
			return scanCode == 0x1D ? CCKEY_LCTRL : CCKEY_RCTRL;
		case VK_ALT:
			return scanCode == 0x38 ? CCKEY_LALT  : CCKEY_RALT;
		case VK_ENTER:
			return scanCode == 0x1C ? CCKEY_ENTER : CCKEY_KP_ENTER;
		default:
			return key < Array_Elems(key_map) ? key_map[key] : 0;
		}
	}
	return 0;
}

// Window procedure
MRESULT EXPENTRY ClientWndProc(HWND hwnd, ULONG message, MPARAM mp1, MPARAM mp2) {
//printf("m %lu\n", message);
	switch(message) {
		case WM_CLOSE:
			if (hwnd == hwndClient)
				WinPostMsg(hwnd, WM_QUIT, 0L, 0L);
				break;
				
		case WM_QUIT:
			
		default: 
			return WinDefWindowProc(hwnd, message, mp1, mp2);
	}
	return FALSE;
}

void DiveFreeBuffer(void) {
	ULONG rc;
	
	if (bufNum != 0) {
		rc = DiveFreeImageBuffer(hDive, bufNum);
		if (rc != DIVE_SUCCESS) {
			// TODO Debug messages
		}
		bufNum = 0;
		
		if (imageBuffer != NULL) {
			rc = DosFreeMem(imageBuffer);
			if (rc != NO_ERROR) {
		  	// TODO Debug messages
			}
			imageBuffer = NULL;
		}
	}
}


/*########################################################################################################################*
*--------------------------------------------------Public implementation--------------------------------------------------*
*#########################################################################################################################*/
void Window_Init(void) { 
	ULONG rc;
	PPIB pib;
	DIVE_CAPS caps = {0};
	FOURCC fccFormats[100] = {0};
	
printf("Window_Init\n");
	/* Change process type code for use Win* API from VIO session */
	DosGetInfoBlocks(NULL, &pib);
	if (pib->pib_ultype == 2 || pib->pib_ultype == 0) {
			/* VIO windowable or fullscreen protect-mode session */
			pib->pib_ultype = 3; /* Presentation Manager protect-mode session */
	}

	habAnchor = WinInitialize(0);
	if (habAnchor == NULLHANDLE) {
		Logger_Abort2(LOUSHORT(WinGetLastError(0)), "Initialization failed");
	}
	msgQueue = WinCreateMsgQueue(habAnchor, 0);
	if (msgQueue == NULLHANDLE) {
		Logger_Abort2(LOUSHORT(WinGetLastError(habAnchor)), "Window queue creation failed");
	}
	
	// Init Dive
	caps.pFormatData = fccFormats;
  caps.ulFormatLength = 120;
  caps.ulStructLen = sizeof(DIVE_CAPS);
	if (rc = DiveQueryCaps(&caps, DIVE_BUFFER_SCREEN)) {
		Logger_Abort2(rc, "DIVE: Could get capabilities.");
	}
printf("depth %d\n", caps.ulDepth);
	rc = DiveOpen(&hDive, FALSE, NULL);
	if (rc != DIVE_SUCCESS) {
		Logger_Abort2(rc, "DIVE: Display engine instance open failed");
	}
}

void Window_Create(int width, int height) {
	ULONG ulFlags = FCF_TITLEBAR | FCF_SYSMENU | FCF_SHELLPOSITION | FCF_TASKLIST
		| FCF_MINBUTTON | FCF_MAXBUTTON | FCF_SIZEBORDER | FCF_ICON;
		
	if (!WinRegisterClass(habAnchor, CC_WIN_CLASSNAME, ClientWndProc, 
			CS_SIZEREDRAW | CS_MOVENOTIFY | CS_SYNCPAINT, 0)) {
		Logger_Abort2(LOUSHORT(WinGetLastError(habAnchor)), "Window class registration failed");
	}
	hwndFrame = WinCreateStdWindow(HWND_DESKTOP, WS_VISIBLE, &ulFlags, 
		CC_WIN_CLASSNAME, "ClassiCube", 0L, NULLHANDLE, 0, &hwndClient);
	if (hwndFrame == NULLHANDLE) {
		Logger_Abort2(LOUSHORT(WinGetLastError(habAnchor)), "Failed to create window");
	}
	Window_SetSize(width, height);
	Window_Main.Exists = true;
	Window_Main.Handle = (HWND)hwndFrame;
}

void Window_Create2D(int width, int height) { 
	printf("Window_Create2D\n");
	Window_Create(width, height); 
}

void Window_Create3D(int width, int height) { 
	Window_Create(width, height); 
}

void Window_Free(void) {
	DiveFreeBuffer();
	if (hDive != NULLHANDLE) DiveClose(hDive);
}

void Window_ProcessEvents(double delta) {
	QMSG msg;
	HWND hwndFocused;
	
	if (WinPeekMsg(habAnchor, &msg, NULLHANDLE, 0, 0, PM_NOREMOVE)) {
		WinGetMsg(habAnchor, &msg, NULLHANDLE, 0, 0);
		WinDispatchMsg(habAnchor, &msg);
	}
}

void Window_SetTitle(const cc_string* title) { 
	WinSetWindowText(hwndFrame, title->buffer);
}

void Clipboard_GetText(cc_string* value) {
	PSZ data;
	
	if (WinOpenClipbrd(habAnchor)) {
		data = (PSZ) WinQueryClipbrdData(habAnchor, CF_TEXT);
		if (data) { 
			strcpy(value->buffer, data);
			value->length = strlen(data);
		}
		WinCloseClipbrd(habAnchor);
	}
}

void Clipboard_SetText(const cc_string* value) {
	
	if (WinOpenClipbrd(habAnchor)) {
		WinEmptyClipbrd(habAnchor);
		WinSetClipbrdData(habAnchor, (ULONG) value->buffer, CF_TEXT, CFI_POINTER);
		WinCloseClipbrd(habAnchor);
	}
}

void Window_Show(void) { 
	WinShowWindow(hwndFrame, TRUE);
	WinSetFocus(HWND_DESKTOP, hwndFrame);
}

void Window_SetSize(int width, int height) { 
	WinSetWindowPos(hwndFrame, HWND_TOP, 0, 0, width, height, SWP_SIZE);
}

void Cursor_SetPosition(int x, int y) { 
	WinSetWindowPos(hwndFrame, HWND_TOP, x, y, 0, 0, SWP_MOVE);
}

void Window_RequestClose(void) { 
	WinPostMsg(hwndFrame, WM_CLOSE, 0, 0);
}

int Window_GetWindowState(void) {
	ULONG style = WinQueryWindowULong(hwndFrame, QWL_STYLE);
	if (style & WS_MINIMIZED) return WINDOW_STATE_MINIMISED;
	if (style & WS_MAXIMIZED) return WINDOW_STATE_FULLSCREEN;
	return WINDOW_STATE_NORMAL;
}

void Window_AllocFramebuffer(struct Bitmap* bmp) {
	ULONG   rc;
	ULONG   scanLineSize = bmp->width * (32 >> 3);
	
	/* Destroy previous buffer. */
	DiveFreeBuffer();
	
	if (bmp->width == 0 || bmp->height == 0) return;
	
	/* Bytes per line. */
	scanLineSize  = (scanLineSize + 3) & ~3; /* 4-byte aligning */
	
	rc = DosAllocMem((PPVOID)&imageBuffer,
		 (bmp->height * scanLineSize) + sizeof(ULONG),
		 PAG_COMMIT | PAG_READ | PAG_WRITE);
	if (rc != NO_ERROR) {
			// TODO Debug messages
			return;
	}
	
	rc = DiveAllocImageBuffer(hDive, &bufNum,
		mmioStringToFOURCC("RGB4", MMIO_TOUPPER), bmp->width, bmp->height,
		scanLineSize, imageBuffer);
	if (rc != DIVE_SUCCESS) {
printf("DiveAllocImageBuffer(), rc = 0x%lX", rc );
			DosFreeMem(imageBuffer);
			imageBuffer = NULL;
			bufNum = 0;
			return;
	}
	
	/*debug(SDL_LOG_CATEGORY_VIDEO, "buffer: 0x%p, DIVE buffer number: %lu",
				imageBuffer, bufNum );*/
	
	bmp->scan0 = (unsigned int*)imageBuffer;

printf("Window_AllocFrameBuffer\n"); 
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) { 
printf("Window_DrawFrameBuffer\n");  
DiveBlitImage ( hDive,
                bufNum,
                DIVE_BUFFER_SCREEN );

}

void Window_FreeFramebuffer(struct Bitmap* bmp) { 
	DiveFreeBuffer();
	printf("Window_FreeFrameBuffer\n");
}



cc_result Window_EnterFullscreen(void) {
printf("Window_EnterFullscreen\n");
}

cc_result Window_ExitFullscreen(void) { 
printf("Window_ExitFillscreen\n");
}


void Cursor_GetRawPos(int *x, int *y) {
printf("Cursor_GetRawPos\n");
}

void Cursor_DoSetVisible(cc_bool visible) { printf("Cursor_DoSetVisible\n");}

int Window_IsObscured(void) { return 0; }


void ShowDialogCore(const char *title, const char *name) { 
	printf("ShowDialogcore\n");
	WinMessageBox(HWND_DESKTOP, HWND_DESKTOP, name, title, 0, MB_OK);
}

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args) {
#if defined CC_BUILD_OS2
	FILEDLG fileDialog;
	HWND hDialog;

	memset(&fileDialog, 0, sizeof(FILEDLG));
	fileDialog.cbSize = sizeof(FILEDLG);
	fileDialog.fl = FDS_HELPBUTTON | FDS_CENTER | FDS_PRELOAD_VOLINFO | FDS_OPEN_DIALOG;
	fileDialog.pszTitle = (PSZ)args->description;
	fileDialog.pszOKButton = NULL;
	fileDialog.pfnDlgProc = WinDefFileDlgProc;

	Mem_Copy(fileDialog.szFullFile, *args->filters, CCHMAXPATH);
	hDialog = WinFileDlg(HWND_DESKTOP, 0, &fileDialog);
	if (fileDialog.lReturn == DID_OK) {
		cc_string temp = String_FromRaw(fileDialog.szFullFile, CCHMAXPATH); 
		args->Callback(&temp);
	}
	
	return 0;
#else
	return ERR_NOT_SUPPORTED;
#endif
}

cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args) {
#if defined CC_BUILD_OS2
	FILEDLG fileDialog;
	HWND hDialog;

	memset(&fileDialog, 0, sizeof(FILEDLG));
	fileDialog.cbSize = sizeof(FILEDLG);
	fileDialog.fl = FDS_HELPBUTTON | FDS_CENTER | FDS_PRELOAD_VOLINFO | FDS_SAVEAS_DIALOG;
	fileDialog.pszTitle = (PSZ)args->titles;
	fileDialog.pszOKButton = NULL;
	fileDialog.pfnDlgProc = WinDefFileDlgProc;

	Mem_Copy(fileDialog.szFullFile, *args->filters, CCHMAXPATH);
	hDialog = WinFileDlg(HWND_DESKTOP, 0, &fileDialog);
	if (fileDialog.lReturn == DID_OK) {
		cc_string temp = String_FromRaw(fileDialog.szFullFile, CCHMAXPATH);
		args->Callback(&temp);
	}
	
	return 0;
#else
	return ERR_NOT_SUPPORTED;
#endif
}

void Window_OpenKeyboard(struct OpenKeyboardArgs* args) { }
void Window_SetKeyboardText(const cc_string* text) { }
void Window_CloseKeyboard(void) { }

void Window_LockLandscapeOrientation(cc_bool lock) { }

void Window_EnableRawMouse(void) { }

void Window_UpdateRawMouse(void) { }

void Window_DisableRawMouse(void) { }


#endif
