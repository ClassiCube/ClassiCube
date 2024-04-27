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
//static HDC win_DC;
//static cc_bool suppress_resize;
//static int win_totalWidth, win_totalHeight; /* Size of window including titlebar and borders */
//static cc_bool grabCursor;
//static int windowX, windowY;
FOURCC fccColorFormat;

// -- DEBUG VARIABLES
ULONG bufferSize = 0;
// ------------------

static const cc_uint8 key_map[14 * 16] = { 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, '0', '1', '2',
	'3', '4', '5', '6', '7', '8', '9', 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 'A', 'B', 'C', 'D',
	'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
  'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 0	
};

static int MapNativeKey(MPARAM mp1, MPARAM mp2) {
	USHORT flags = SHORT1FROMMP(mp1);
	USHORT scanCode = CHAR4FROMMP(mp1);

//printf("virtualkey %u kc_char %u scancode %u key %u char %u\n",flags&KC_VIRTUALKEY,flags&KC_CHAR,scanCode,vkey,char_);
if (flags & KC_CHAR) {
	//printf("key %c\n",SHORT1FROMMP(mp2));
	return SHORT1FROMMP(mp2);
}

	if (flags & KC_VIRTUALKEY)
	{
		USHORT vkey = SHORT2FROMMP(mp2);
		switch (vkey)
		{
		case VK_CTRL:
			return scanCode == 0x1D ? CCKEY_LCTRL : CCKEY_RCTRL;
		case VK_ALT:
			return scanCode == 0x38 ? CCKEY_LALT  : CCKEY_RALT;
		case VK_ENTER:
			return scanCode == 0x1C ? CCKEY_ENTER : CCKEY_KP_ENTER;
		}
		printf("KC_VIRTUALKEY not mapped %u\n", vkey);
	}
	
	return 0;
}

static void RefreshWindowBounds() {
	RECTL rect;
	
	if (WinQueryWindowRect(hwndClient, &rect)) {
		Window_Main.Width = Rect_Width(rect);
		Window_Main.Height = Rect_Height(rect);
	}
}

// Window procedure
MRESULT EXPENTRY ClientWndProc(HWND hwnd, ULONG message, MPARAM mp1, MPARAM mp2) {
//printf("m %lu (%x)\n", message, message);
	switch(message) {
		case WM_CLOSE:
			if (hwnd == hwndClient) {
        WinSendMsg(hwndFrame, WM_SetVideoMode, (MPARAM)WS_DesktopDive, 0);
				WinPostMsg(hwnd, WM_QUIT, 0L, 0L);
			}
			break;

		case WM_QUIT:
         Window_Free();
				 Window_Main.Handle = NULL;
				 Window_Main.Exists = false;
				 break;
				
		case WM_SIZE:
			RefreshWindowBounds();
			Event_RaiseVoid(&WindowEvents.RedrawNeeded);
			break;
			
		case WM_MOVE:
			Event_RaiseVoid(&WindowEvents.RedrawNeeded);
			break;
		
		case WM_VRNDISABLED:
      fVrnDisabled = TRUE;
      return FALSE;
      break;

    case WM_VRNENABLED: {
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
			
		case WM_MOUSEMOVE: {
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
			
		case WM_MATCHMNEMONIC:
			return (MRESULT)TRUE;
		case WM_CHAR: {
//printf("WM_CHAR\n");
			USHORT flags = SHORT1FROMMP(mp1);

			int mappedKey = MapNativeKey(mp1, mp2);
			if (mappedKey != 0) {
				printf("key mapped %c => %c\n", CHAR4FROMMP(mp1), mappedKey);
				Input_SetPressed(mappedKey/*, (flags & KC_KEYUP) == 0*/);
				break;
			}
		}
		break;
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
		Logger_Abort2(LOUSHORT(WinGetLastError(0)), "Initialization failed");
	}
	msgQueue = WinCreateMsgQueue(habAnchor, 0);
	if (msgQueue == NULLHANDLE) {
		Logger_Abort2(LOUSHORT(WinGetLastError(habAnchor)), "Window queue creation failed");
	}

	// Init Dive
	caps.pFormatData = fccFormats;
  caps.ulFormatLength = 100;
  caps.ulStructLen = sizeof(DIVE_CAPS);
	if (rc = DiveQueryCaps(&caps, DIVE_BUFFER_SCREEN)) {
		Logger_Abort2(rc, "DIVE: Could not get capabilities.");
	}
  if (caps.ulDepth < 24) {
  	Logger_Abort2(rc, "DIVE: Too few colours");
  }

	rc = DiveOpen(&hDive, FALSE, NULL);
	if (rc != DIVE_SUCCESS) {
		Logger_Abort2(rc, "DIVE: Display engine instance open failed");
	}
	// Everything is fine, so we set infos.
	hps = WinGetScreenPS(HWND_DESKTOP);
	hdc = GpiQueryDevice(hps);
	DevQueryCaps(hdc, CAPS_FAMILY, CAPS_DEVICE_POLYSET_POINTS, screencaps);
  WinReleasePS(hps);
	DisplayInfo.Width = screencaps[CAPS_WIDTH];
	DisplayInfo.Height = screencaps[CAPS_HEIGHT];
	DisplayInfo.Depth = screencaps[CAPS_COLOR_BITCOUNT];
	// OS/2 gives the resolution per meter
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
	DisplayInfo.DPIScaling = false; /*Options_GetBool(OPT_DPI_SCALING, false)*/
	Input.Sources = INPUT_SOURCE_NORMAL;
}

void Window_Create(int width, int height) {
	ULONG ulFlags = FCF_TITLEBAR | FCF_SYSMENU | FCF_SHELLPOSITION | FCF_TASKLIST
		| FCF_MINBUTTON | FCF_MAXBUTTON | FCF_SIZEBORDER | FCF_ICON;

	if (!WinRegisterClass(habAnchor, CC_WIN_CLASSNAME, ClientWndProc, 0, 0)) {
		Logger_Abort2(LOUSHORT(WinGetLastError(habAnchor)), "Window class registration failed");
	}
	hwndFrame = WinCreateStdWindow(HWND_DESKTOP, WS_VISIBLE | WS_SYNCPAINT, &ulFlags,
		CC_WIN_CLASSNAME, "ClassiCube", 0L, NULLHANDLE, 0, &hwndClient);
	if (hwndFrame == NULLHANDLE) {
		Logger_Abort2(LOUSHORT(WinGetLastError(habAnchor)), "Failed to create window");
	}
	Window_SetSize(width, height);
	Window_Main.Exists = true;
	Window_Main.Handle = &hwndFrame;
	
  WinSetVisibleRegionNotify(hwndClient, TRUE);
	// Send an invlaidation message to the client.
	WinPostMsg(hwndFrame, WM_VRNENABLED, 0L, 0L);
}

void Window_Create2D(int width, int height) {
	printf("Window_Create2D\n");
	Window_Create(width, height);
}

void Window_Create3D(int width, int height) {
	Window_Create(width, height);
}

void Window_Free(void) {
	WinSetVisibleRegionNotify (hwndClient, FALSE);
	DiveFreeBuffer();
	if (hDive != NULLHANDLE) DiveClose(hDive);
}

void Window_ProcessEvents(float delta) {
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
	APIRET rc;
	ULONG scanLineSize = bmp->width * (32 >> 3);

	/* Destroy previous buffer. */
	DiveFreeBuffer();

	if (bmp->width == 0 || bmp->height == 0) return;

	/* Bytes per line. */
	scanLineSize  = (scanLineSize + 3) & ~3; /* 4-byte aligning */

	rc = DosAllocMem((PVOID)&imageBuffer,
		 bufferSize = (bmp->height * scanLineSize) + sizeof(ULONG),
		 PAG_COMMIT | PAG_READ | PAG_WRITE);
	if (rc != NO_ERROR) {
			// TODO Debug messages
			return;
	}

	rc = DiveAllocImageBuffer(hDive, &bufNum, FOURCC_BGR4, bmp->width, bmp->height,
		scanLineSize, imageBuffer);
	if (rc != DIVE_SUCCESS) {
			DosFreeMem(imageBuffer);
			imageBuffer = NULL;
			bufNum = 0;
			bmp->scan0 = NULL;
			Logger_Abort2(rc, "Dive: Could not allocate image buffer");
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
		Logger_Abort2(rc, "Dive: Access to image buffer failed");
	}

	bmp->scan0 = (unsigned int*)imageBuffer;
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
	WinMessageBox(HWND_DESKTOP, hwndClient, name, title, 0, MB_OK|MB_APPLMODAL);
}                                                           

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args) {
	FILEDLG fileDialog;
	HWND hDialog;

	memset(&fileDialog, 0, sizeof(FILEDLG));
	fileDialog.cbSize = sizeof(FILEDLG);
	fileDialog.fl = FDS_HELPBUTTON | FDS_CENTER | FDS_PRELOAD_VOLINFO | FDS_OPEN_DIALOG;
	fileDialog.pszTitle = (PSZ)args->description;
	fileDialog.pszOKButton = NULL;
	fileDialog.pfnDlgProc = WinDefFileDlgProc;

	Mem_Copy(fileDialog.szFullFile, *args->filters, CCHMAXPATH);
	hDialog = WinFileDlg(HWND_DESKTOP, hwndFrame, &fileDialog);
	if (fileDialog.lReturn == DID_OK) {
		cc_string temp = String_FromRaw(fileDialog.szFullFile, CCHMAXPATH);
		args->Callback(&temp);
	}

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
	fileDialog.pfnDlgProc = WinDefFileDlgProc;

	Mem_Copy(fileDialog.szFullFile, *args->filters, CCHMAXPATH);
	hDialog = WinFileDlg(HWND_DESKTOP, hwndFrame, &fileDialog);
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
	printf("Window_EnableRawMouse");
}

void Window_UpdateRawMouse(void) { }

void Window_DisableRawMouse(void) { }

void Window_ProcessGamepads(float delta) { }

#endif
