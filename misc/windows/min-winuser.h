/* Compatibility so that compiling in older windows SDK versions works */

#ifndef WM_MOUSEWHEEL
	#define WM_MOUSEWHEEL 0x020A
#endif
/* Only present if WIN32_WINNT >= 0x0500 */
#ifndef WM_XBUTTONDOWN
	#define WM_XBUTTONDOWN 0x020B
	#define WM_XBUTTONUP   0x020C
#endif
/* Only present if WIN32_WINNT >= 0x0501 */
#ifndef WM_INPUT
	#define WM_INPUT       0x00FF
#endif

/* Only present if WIN32_WINNT >= 0x0600 */
#ifndef WM_MOUSEHWHEEL
	#define WM_MOUSEHWHEEL 0x020E
#endif

/* Only present if WIN32_WINNT >= 0x0500 */
#ifndef SM_CXVIRTUALSCREEN
#define SM_CXVIRTUALSCREEN      78
#define SM_CYVIRTUALSCREEN      79
#endif

#ifndef WHEEL_DELTA
	#define WHEEL_DELTA 120
#endif
/* Only present if WIN32_WINNT >= 0x0501 */
#ifndef MOUSE_MOVE_RELATIVE
	#define MOUSE_MOVE_RELATIVE      0
	#define MOUSE_MOVE_ABSOLUTE      1
	#define MOUSE_VIRTUAL_DESKTOP 0x02
#endif

/* Only present if WIN32_WINNT >= 0x0501 */
#ifndef RIM_TYPEMOUSE
	#define RIM_TYPEMOUSE 0
	#define RIDEV_INPUTSINK 0x00000100
#endif

#ifndef RID_INPUT
	#define RID_INPUT 0x10000003
#endif
typedef struct _RAWINPUTDEVICE {
	USHORT usUsagePage;
	USHORT usUsage;
	DWORD dwFlags;
	HWND hwndTarget;
} RAWINPUTDEVICE;

typedef struct _RAWINPUTHEADER {
	DWORD dwType;
	DWORD dwSize;
	HANDLE hDevice;
	WPARAM wParam;
} RAWINPUTHEADER;

typedef struct _RAWMOUSE {
	USHORT usFlags;
	union {
		ULONG ulButtons;
		struct {
			USHORT usButtonFlags;
			USHORT usButtonData;
		} DUMMYSTRUCTNAME;
	} DUMMYUNIONNAME;
	ULONG ulRawButtons;
	LONG lLastX;
	LONG lLastY;
	ULONG ulExtraInformation;
} RAWMOUSE;

typedef struct _RAWKEYBOARD {
	USHORT MakeCode;
	USHORT Flags;
	USHORT Reserved;
	USHORT VKey;
	UINT Message;
	ULONG ExtraInformation;
} RAWKEYBOARD;

typedef struct _RAWHID {
	DWORD dwSizeHid;
	DWORD dwCount;
	BYTE bRawData[1];
} RAWHID;

typedef struct _RAWINPUT {
	RAWINPUTHEADER header;
	union {
		RAWMOUSE mouse;
		RAWKEYBOARD keyboard;
		RAWHID hid;
	} data;
} RAWINPUT;

typedef void* HRAWINPUT;

