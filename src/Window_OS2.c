#include "Core.h"
#if defined CC_BUILD_OS2 && !defined CC_BUILD_SDL2
#include "_WindowBase.h"
#include "String_.h"
#include "Funcs.h"
#include "Bitmap.h"
#include "Options.h"
#include "Errors.h"
#include "Audio.h"

#define INCL_DOSPROCESS
#define INCL_DOSMEMMGR
#define INCL_DOSERRORS
#define INCL_PM
#define INCL_MMIOOS2
#define INCL_GRE_COLORTABLE
#define INCL_GRE_DEVMISC3
#include <os2.h>
#include <os2me.h>
#include <dive.h>
#include <fourcc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define WM_GetVideoModeTable  0x04A2
#define WM_SetVideoMode       0x04A0
#define WM_NotifyVideoModeChange 0x04A1
#define WS_DesktopDive    0x00000000L
#define WS_MaxDesktopDive 0x00000001L
#define WS_FullScreenDive 0x00000002L

#define CC_WIN_CLASSNAME "ClassiCube_Window"
#define Rect_Width(rect)  (rect.xRight  - rect.xLeft)
#define Rect_Height(rect) (rect.yTop - rect.yBottom)

static HAB habAnchor;
static HMQ msgQueue;
static HWND hwndFrame;
static HWND hwndClient;
static HDIVE hDive = NULLHANDLE;
static ULONG bufNum = 0;
static PBYTE imageBuffer = NULL;
BOOL fVrnDisabled = TRUE;
static ULONG bufferScanLineSize, bufferScanLines;
static ULONG clrColor;
RECTL rcls[50];
ULONG ulNumRcls;
static int win_width, win_height; /* Size of window including titlebar and borders */
static int windowX, windowY;
FOURCC fccColorFormat;

FILEDLG fileDialog;
void *mutex = NULL;

/**
 * Handling native keystrokes
 * Borrowed from SDL2 OS/2 code
 **/
static void MapKeys(MPARAM mp1, MPARAM mp2) {
	ULONG   ulFlags = SHORT1FROMMP(mp1);      /* WM_CHAR flags         */
	ULONG   ulVirtualKey = SHORT2FROMMP(mp2); /* Virtual key code VK_* */
	ULONG   ulCharCode = SHORT1FROMMP(mp2);   /* Character code        */
	ULONG   ulScanCode = CHAR4FROMMP(mp1);    /* Scan code             */
	static const int aScancode[] = {
		INPUT_NONE, CCKEY_ESCAPE, CCKEY_1, CCKEY_2, CCKEY_3, CCKEY_4, CCKEY_5, CCKEY_6,
		CCKEY_7, CCKEY_8, CCKEY_9, CCKEY_0, CCKEY_MINUS, CCKEY_EQUALS, CCKEY_BACKSPACE, CCKEY_TAB,
		CCKEY_Q, CCKEY_W, CCKEY_E, CCKEY_R, CCKEY_T, CCKEY_Y, CCKEY_U, CCKEY_I,
		CCKEY_O, CCKEY_P, CCKEY_LBRACKET, CCKEY_RBRACKET, CCKEY_ENTER, CCKEY_LCTRL, CCKEY_A, CCKEY_S,
		CCKEY_D, CCKEY_F, CCKEY_G, CCKEY_H, CCKEY_J, CCKEY_K, CCKEY_L, CCKEY_SEMICOLON,
		INPUT_NONE, INPUT_NONE, CCKEY_LSHIFT, CCKEY_BACKSLASH, CCKEY_Z, CCKEY_X, CCKEY_C, CCKEY_V,
		CCKEY_B, CCKEY_N, CCKEY_M, CCKEY_COMMA, CCKEY_PERIOD, CCKEY_SLASH, CCKEY_RSHIFT, CCKEY_KP_MULTIPLY,
		CCKEY_LALT, CCKEY_SPACE, CCKEY_CAPSLOCK, CCKEY_F1, CCKEY_F2, CCKEY_F3, CCKEY_F4, CCKEY_F5,
		CCKEY_F6, CCKEY_F7, CCKEY_F8, CCKEY_F9, CCKEY_F10, CCKEY_NUMLOCK, CCKEY_SCROLLLOCK, CCKEY_KP7,
		CCKEY_KP8, CCKEY_KP9, CCKEY_KP_MINUS, CCKEY_KP4, CCKEY_KP5, CCKEY_KP6, CCKEY_KP_PLUS, CCKEY_KP1,
		CCKEY_KP2, CCKEY_KP3, CCKEY_KP0, CCKEY_KP_DECIMAL, INPUT_NONE, INPUT_NONE, CCKEY_BACKSLASH,CCKEY_F11,
		CCKEY_F12, CCKEY_PAUSE, CCKEY_KP_ENTER, CCKEY_RCTRL, CCKEY_KP_DIVIDE, INPUT_NONE, CCKEY_RALT, INPUT_NONE,
		CCKEY_HOME, CCKEY_UP, CCKEY_PAGEUP, CCKEY_LEFT, CCKEY_RIGHT, CCKEY_END, CCKEY_DOWN, CCKEY_PAGEDOWN,
		CCKEY_F17, CCKEY_DELETE, CCKEY_F19, INPUT_NONE, INPUT_NONE, INPUT_NONE, INPUT_NONE, INPUT_NONE,
		INPUT_NONE, INPUT_NONE, INPUT_NONE, INPUT_NONE,INPUT_NONE, INPUT_NONE, INPUT_NONE, INPUT_NONE,
		INPUT_NONE, INPUT_NONE, INPUT_NONE, INPUT_NONE,INPUT_NONE,INPUT_NONE,CCKEY_LWIN, CCKEY_RWIN
	};
	
	if ((ulFlags & KC_CHAR) != 0 && (ulFlags & KC_KEYUP) == 0) {
		if (WinQueryCapture(HWND_DESKTOP) != hwndClient) { // Mouse isn't captured
			char c;
			if (Convert_TryCodepointToCP437(ulCharCode, &c)) {
				Event_RaiseInt(&InputEvents.Press, c);
				return;
			}
		}
		else {
			Input_Set(ulCharCode, 0);
		}
	}
	if ((ulFlags & KC_SCANCODE) != 0 && ulScanCode < sizeof(aScancode)/sizeof(int)) {
		Input_Set(aScancode[ulScanCode], (ulFlags & KC_KEYUP) == 0);
	}
}

static void RefreshWindowBounds() {
	RECTL rect;
	cc_bool changed = false;

	if (WinQueryWindowRect(hwndClient, &rect)) {
		Window_Main.Width = Rect_Width(rect);
		Window_Main.Height = Rect_Height(rect);
		if (Window_Main.Width != win_width || Window_Main.Height != win_height) {
			changed = true;
			win_width = Window_Main.Width;
			win_height = Window_Main.Height;
		}
		if (changed) {
			Event_RaiseVoid(&WindowEvents.StateChanged);
			Event_RaiseVoid(&WindowEvents.Resized);
		}
	}
}

// Window procedure
MRESULT EXPENTRY ClientWndProc(HWND hwnd, ULONG message, MPARAM mp1, MPARAM mp2) {
//printf("m %lu (%x)\n", message, message);
	switch(message) {
	   case WM_CLOSE:
		   Event_RaiseVoid(&WindowEvents.Closing);
		   if (Window_Main.Exists) WinDestroyWindow(hwndFrame);
		   Window_Main.Exists = false;
	   	break;

		case WM_QUIT:
			Window_Destroy();
			Window_Main.Handle.ptr = NULL;
			Window_Main.Exists = false;
			return (MRESULT)0;

		case WM_ACTIVATE:
			Window_Main.Focused = SHORT1FROMMP(mp1) != 0;
			Event_RaiseVoid(&WindowEvents.FocusChanged);
			break;
			
		case WM_SIZE:
			RefreshWindowBounds();
			break;

		case WM_MOVE:
			RefreshWindowBounds();
			break;

		case WM_ERASEBACKGROUND:
			return (MRESULT)TRUE;
			
		case WM_VRNDISABLED:
      fVrnDisabled = TRUE;
      return FALSE;
      break;

    case WM_VRNENABLED:
    	{
    		HPS hps;
    		HRGN hrgn;
    		RGNRECT  rgnCtl;

				hps = WinGetPS(hwnd);
				hrgn = GpiCreateRegion(hps, 0L, NULL);
				if (hrgn) {
					// NOTE: If mp1 is zero, then this was just a move message.
					// Illustrate the visible region on a WM_VRNENABLE.
					//
					WinQueryVisibleRegion(hwndClient, hrgn);
					rgnCtl.ircStart     = 0;
					rgnCtl.crc          = 50;
					rgnCtl.ulDirection  = 1;

					// Get the all visible rectangles
					if(!GpiQueryRegionRects(hps, hrgn, NULL, &rgnCtl, rcls)) {
						 DiveSetupBlitter (hDive, 0);
						 GpiDestroyRegion (hps, hrgn);
						 break;
					}

					ulNumRcls = rgnCtl.crcReturned;
					GpiDestroyRegion(hps, hrgn);

					// Release presentation space
					WinReleasePS(hps);
				}
				if (mp1 == 0) Event_RaiseVoid(&WindowEvents.RedrawNeeded);
				// Enable blitting
				fVrnDisabled = FALSE;
			}
			return FALSE;
      break;

		case WM_MOUSEMOVE:
			{
				SHORT x = SHORT1FROMMP(mp1);
				SHORT y = Window_Main.Height - SHORT2FROMMP(mp1);
				Pointer_SetPosition(0, x, y);
			}
			break;
			
		case WM_BUTTON1DOWN:
			Input_SetPressed(CCMOUSE_L); break;
		case WM_BUTTON3DOWN:
			Input_SetPressed(CCMOUSE_M); break;
		case WM_BUTTON2DOWN:
			Input_SetPressed(CCMOUSE_R); break;

		case WM_BUTTON1UP:
			Input_SetReleased(CCMOUSE_L); break;
		case WM_BUTTON3UP:
			Input_SetReleased(CCMOUSE_M); break;
		case WM_BUTTON2UP:
			Input_SetReleased(CCMOUSE_R); break;

		case WM_CHAR:
			MapKeys(mp1, mp2);
			return (MRESULT)TRUE;
	}
//printf("default msg %lu (%x)\n", message, message);
	return WinDefWindowProc(hwnd, message, mp1, mp2);
}

void DiveFreeBuffer(void) {
	ULONG rc;

	if (bufNum != 0) {
		rc = DiveFreeImageBuffer(hDive, bufNum);
		if (rc != DIVE_SUCCESS) {
			// TODO Error messages
		}
		bufNum = 0;
	}
	if (imageBuffer != NULL) {
		rc = DosFreeMem(imageBuffer);
		if (rc != NO_ERROR) {
			// TODO Error messages
		}
		imageBuffer = NULL;
	}
}


/*########################################################################################################################*
*--------------------------------------------------Public implementation--------------------------------------------------*
*#########################################################################################################################*/
void Window_PreInit(void) { }
void Window_Init(void) {
	ULONG rc;
	PPIB pib;
	DIVE_CAPS caps = {0};
	FOURCC fccFormats[100] = {0};
	HDC hdc;
	HPS hps;
	LONG screencaps[CAPS_DEVICE_POLYSET_POINTS];

	/* Change process type code for use Win* API from VIO session */
	DosGetInfoBlocks(NULL, &pib);
	if (pib->pib_ultype == 2 || pib->pib_ultype == 0) {
			/* VIO windowable or fullscreen protect-mode session */
			pib->pib_ultype = 3; /* Presentation Manager protect-mode session */
	}

	habAnchor = WinInitialize(0);
	if (habAnchor == NULLHANDLE) {
		Process_Abort2(LOUSHORT(WinGetLastError(0)), "Initialization failed");
	}
	msgQueue = WinCreateMsgQueue(habAnchor, 0);
	if (msgQueue == NULLHANDLE) {
		Process_Abort2(LOUSHORT(WinGetLastError(habAnchor)), "Window queue creation failed");
	}

	// Init Dive
	caps.pFormatData = fccFormats; //TODO do something with it.
  caps.ulFormatLength = 100;
  caps.ulStructLen = sizeof(DIVE_CAPS);
	if (rc = DiveQueryCaps(&caps, DIVE_BUFFER_SCREEN)) {
		Process_Abort2(rc, "DIVE: Could not get capabilities.");
	}
  if (caps.ulDepth < 24) {
  	Process_Abort2(rc, "DIVE: Too few colours");
  }

	rc = DiveOpen(&hDive, FALSE, NULL);
	if (rc != DIVE_SUCCESS) {
		Process_Abort2(rc, "DIVE: Display engine instance open failed");
	}
	// Everything is fine, so we set infos.
	hps = WinGetScreenPS(HWND_DESKTOP);
	hdc = GpiQueryDevice(hps);
	DevQueryCaps(hdc, CAPS_FAMILY, CAPS_DEVICE_POLYSET_POINTS, screencaps);
  WinReleasePS(hps);
	DisplayInfo.Width = screencaps[CAPS_WIDTH];
	DisplayInfo.Height = screencaps[CAPS_HEIGHT];
	DisplayInfo.Depth = screencaps[CAPS_COLOR_BITCOUNT];
	DisplayInfo.ScaleX = 1.0;
	DisplayInfo.ScaleY = 1.0;
	DisplayInfo.DPIScaling = Options_GetBool(OPT_DPI_SCALING, false);
	Input.Sources = INPUT_SOURCE_NORMAL;
}

void Window_Create(int width, int height) {
	ULONG ulFlags = FCF_SHELLPOSITION | FCF_TASKLIST
		| FCF_SIZEBORDER | FCF_ICON | FCF_MAXBUTTON |FCF_MINBUTTON
		| FCF_MAXBUTTON | FCF_TITLEBAR | FCF_SYSMENU;
	
	if (!WinRegisterClass(habAnchor, CC_WIN_CLASSNAME, ClientWndProc, 0, 0)) {
		Process_Abort2(LOUSHORT(WinGetLastError(habAnchor)), "Window class registration failed");
	}
	hwndFrame = WinCreateStdWindow(HWND_DESKTOP, WS_VISIBLE | WS_SYNCPAINT, &ulFlags,
		CC_WIN_CLASSNAME, "ClassiCube", 0L, NULLHANDLE, 0, &hwndClient);
	if (hwndFrame == NULLHANDLE) {
		Process_Abort2(LOUSHORT(WinGetLastError(habAnchor)), "Failed to create window");
	}
	Window_SetSize(width, height);
	Window_Main.Exists = true;
	Window_Main.Handle.ptr = &hwndFrame;
	Window_Main.Focused = TRUE;

  WinSetVisibleRegionNotify(hwndClient, TRUE);
	// Send an invlaidation message to the client.
	WinPostMsg(hwndFrame, WM_VRNENABLED, 0L, 0L);
}

void Window_Create2D(int width, int height) {
	Window_Create(width, height);
}

void Window_Create3D(int width, int height) {
	Window_Create(width, height);
}

void Window_Free(void) { }

void Window_Destroy(void) {
	WinSetVisibleRegionNotify (hwndClient, FALSE);
	DiveFreeBuffer();
	if (hDive != NULLHANDLE) DiveClose(hDive);
	hDive = NULLHANDLE;
   AudioBackend_Free();
}

void Window_ProcessEvents(float delta) {
	QMSG msg;
	HWND hwndFocused;
	int roundtrips = 5;
	
	while (roundtrips && WinPeekMsg(habAnchor, &msg, NULLHANDLE, 0, 0, PM_NOREMOVE)) {
		WinGetMsg(habAnchor, &msg, NULLHANDLE, 0, 0);
		WinDispatchMsg(habAnchor, &msg);
		roundtrips--;
	}
}

void Window_SetTitle(const cc_string* title) {
	char nativeTitle[title->capacity+1];
	Mem_Set(nativeTitle, 0, title->capacity+1);
	String_CopyToRaw(nativeTitle, title->capacity, title);
	WinSetWindowText(hwndFrame, nativeTitle);
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
	WinSetFocus(HWND_DESKTOP, hwndClient);
}

void Window_SetSize(int width, int height) {
	WinSetWindowPos(hwndFrame, HWND_TOP, 0, 0, width, height, SWP_SIZE);
}

void Window_RequestClose(void) {
	WinPostMsg(hwndClient, WM_CLOSE, 0, 0);
}

int Window_GetWindowState(void) {
	ULONG style = WinQueryWindowULong(hwndFrame, QWL_STYLE);
	if (style & WS_MINIMIZED) return WINDOW_STATE_MINIMISED;
	if (style & WS_MAXIMIZED) return WINDOW_STATE_FULLSCREEN;
	return WINDOW_STATE_NORMAL;
}

void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	APIRET rc;
	ULONG scanLineSize = width * (32 >> 3);

	/* Destroy previous buffer. */
	DiveFreeBuffer();

	if (width == 0 || height == 0) return;

	/* Bytes per line. */
	scanLineSize  = (scanLineSize + 3) & ~3; /* 4-byte aligning */

	rc = DosAllocMem((PVOID)&imageBuffer,
		 (height * scanLineSize) + sizeof(ULONG),
		 PAG_COMMIT | PAG_READ | PAG_WRITE);

	if (rc != NO_ERROR) {
			// TODO Debug messages
			return;
	}

	rc = DiveAllocImageBuffer(hDive, &bufNum, FOURCC_BGR4, width, height,
		scanLineSize, imageBuffer);
	if (rc != DIVE_SUCCESS) {
			DosFreeMem(imageBuffer);
			imageBuffer = NULL;
			bufNum = 0;
			bmp->scan0 = NULL;
			Process_Abort2(rc, "Dive: Could not allocate image buffer");
			return;
	}

	rc = DiveBeginImageBufferAccess(hDive, bufNum,
			&imageBuffer, &bufferScanLineSize, &bufferScanLines);
	if (rc != DIVE_SUCCESS) {
		DiveFreeImageBuffer(hDive, bufNum);
		DosFreeMem(imageBuffer);
		imageBuffer = NULL;
		bufNum = 0;
		bmp->scan0 = NULL;
		Process_Abort2(rc, "Dive: Access to image buffer failed");
	}

	bmp->scan0 = (unsigned int*)imageBuffer;
	bmp->width = width;
	bmp->height = height;
}


void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	SETUP_BLITTER setupBlitter;
	POINTL pointl;
	SWP swp;
	RECTL rect;
	APIRET rc;

	rc = DiveEndImageBufferAccess(hDive, bufNum);
	if (rc != DIVE_SUCCESS) {
		// TODO Error messages
		return;
	}
	if (fVrnDisabled) {
		DiveSetupBlitter(hDive, 0);
	}
	else {
		// Now find the window position and size, relative to parent.
		WinQueryWindowPos(hwndClient, &swp);
		// Convert the point to offset from desktop lower left.
		pointl.x = swp.x;
		pointl.y = swp.y;
		WinMapWindowPoints(hwndFrame, HWND_DESKTOP, &pointl, 1);
		WinQueryWindowRect(hwndClient, &rect);

		// TODO CC's coordinate begins in the upper left corner.
		// TODO Fix coordinate system for partial updates
		// Tell DIVE about the new settings.
		setupBlitter.ulStructLen       = sizeof(SETUP_BLITTER);
		setupBlitter.fccSrcColorFormat = FOURCC_BGR4;
		setupBlitter.ulSrcWidth        = swp.cx;
		setupBlitter.ulSrcHeight       = swp.cy;
		setupBlitter.ulSrcPosX         = 0;
		setupBlitter.ulSrcPosY         = 0;
		setupBlitter.fInvert           = FALSE;
		setupBlitter.ulDitherType      = 1;
		setupBlitter.fccDstColorFormat = FOURCC_SCRN;
		setupBlitter.ulDstWidth        = swp.cx;
		setupBlitter.ulDstHeight       = swp.cy;
		setupBlitter.lDstPosX          = 0;
		setupBlitter.lDstPosY          = 0;
		setupBlitter.lScreenPosX       = pointl.x;
		setupBlitter.lScreenPosY       = pointl.y;
		setupBlitter.ulNumDstRects     = ulNumRcls;
		setupBlitter.pVisDstRects      = rcls;
		DiveSetupBlitter(hDive, &setupBlitter);
		DiveBlitImage(hDive, bufNum, DIVE_BUFFER_SCREEN);
	}
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	DiveFreeBuffer();
}

cc_result Window_EnterFullscreen(void) {
	SWP swp;
	if (!WinGetMaxPosition(hwndFrame, &swp)) return ERROR_BASE; // TODO
	if (!WinSetWindowPos(hwndFrame, HWND_TOP,
		swp.x, swp.y, swp.cx, swp.cy, SWP_MAXIMIZE)
	) return ERROR_BASE; // TODO
	return 0;
}

cc_result Window_ExitFullscreen(void) {
	if (!WinSetWindowPos(hwndFrame, HWND_TOP,
		0, 0, 0, 0, SWP_RESTORE | SWP_SHOW | SWP_ACTIVATE)) return ERROR_BASE; // TODO
	return 0;
}

int Window_IsObscured(void) { return 0; }

void ShowDialogCore(const char *title, const char *name) {
	WinMessageBox(HWND_DESKTOP, hwndClient, name, title, 0, MB_OK|MB_APPLMODAL);
}

MRESULT EXPENTRY myFileDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2) {
	MRESULT result;
	printf("Dialog message %04x, mp1 %u, mp2 %u hwnd %u (%u, %u)\n", msg, mp1, mp2, hwnd, hwndFrame, hwndClient);
	result = WinDefFileDlgProc(hwnd, msg, mp1, mp2);
	printf("Default %u\n", result);
	return result;
}

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args) {

	HWND hDialog;
	
	WinSetVisibleRegionNotify(hwndClient, FALSE);
	memset(&fileDialog, 0, sizeof(FILEDLG));
	fileDialog.cbSize = sizeof(FILEDLG);
	fileDialog.fl = FDS_OPEN_DIALOG | FDS_CENTER | FDS_HELPBUTTON;
	fileDialog.pszTitle = (PSZ)args->description;
	//fileDialog.pfnDlgProc = myFileDlgProc;

	strcpy(fileDialog.szFullFile, *(args->filters));
	hDialog = WinFileDlg(HWND_DESKTOP, hwndFrame, &fileDialog);

	if (fileDialog.lReturn == DID_OK) {
		cc_string temp = String_FromRaw(fileDialog.szFullFile, CCHMAXPATH);
		args->Callback(&temp);
	}
	WinSetVisibleRegionNotify(hwndClient, TRUE);
	return 0;
}

cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args) {
	FILEDLG fileDialog;
	HWND hDialog;

	memset(&fileDialog, 0, sizeof(FILEDLG));
	fileDialog.cbSize = sizeof(FILEDLG);
	fileDialog.fl = FDS_HELPBUTTON | FDS_CENTER | FDS_PRELOAD_VOLINFO | FDS_SAVEAS_DIALOG;
	fileDialog.pszTitle = (PSZ)args->titles;
	fileDialog.pszOKButton = NULL;

	Mem_Copy(fileDialog.szFullFile, *(args->filters), CCHMAXPATH);
	hDialog = WinFileDlg(HWND_DESKTOP, hwndClient, &fileDialog);
	if (fileDialog.lReturn == DID_OK) {
		cc_string temp = String_FromRaw(fileDialog.szFullFile, CCHMAXPATH);
		args->Callback(&temp);
	}

	return 0;
}

/* Displays on-screen keyboard for platforms that lack physical keyboard input. */
/* NOTE: On desktop platforms, this won't do anything. */
void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) { }
/* Sets the text used for keyboard input. */
/* NOTE: This is e.g. used for mobile on-screen keyboard input with the web client, */
/*  because it is backed by a HTML input, rather than true keyboard input events. */
/* As such, this is necessary to ensure the HTML input is consistent with */
/*  whatever text input widget is actually being displayed on screen. */
void OnscreenKeyboard_SetText(const cc_string* text) { }
void OnscreenKeyboard_Draw2D(Rect2D* r, struct Bitmap* bmp) { }
void OnscreenKeyboard_Draw3D(void) { }
/* Hides/Removes the previously displayed on-screen keyboard. */
void OnscreenKeyboard_Close(void) { }
/* Locks/Unlocks the landscape orientation. */
void Window_LockLandscapeOrientation(cc_bool lock) { }

void Window_EnableRawMouse(void) {
	if (WinSetCapture(HWND_DESKTOP, hwndClient))
		DefaultEnableRawMouse();
}

void Window_UpdateRawMouse(void) {
	DefaultUpdateRawMouse();
}

void Window_DisableRawMouse(void) {
	if (WinSetCapture(HWND_DESKTOP, NULLHANDLE))
		DefaultDisableRawMouse();
}

void Gamepads_PreInit(void) { }

void Gamepads_Init(void) { }

void Gamepads_Process(float delta) { }

void Cursor_GetRawPos(int *x, int *y) {
    POINTL  pointl;
    ULONG   ulRes;

    WinQueryPointerPos(HWND_DESKTOP, &pointl);
    *x = pointl.x;
    *y = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN) - pointl.y - 1;
}

void Cursor_SetPosition(int x, int y) {
	POINTL      pointl;

  pointl.x = x;
  pointl.y = win_height - y;
  WinMapWindowPoints(hwndClient, HWND_DESKTOP, &pointl, 1);
  WinSetPointerPos(HWND_DESKTOP, pointl.x, pointl.y);
}

static void Cursor_DoSetVisible(cc_bool visible) {
	WinShowCursor(hwndClient, visible);
}



void Window_ProcessGamepads(float delta) { }

#endif
