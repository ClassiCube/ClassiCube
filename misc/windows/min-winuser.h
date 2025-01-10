#ifndef CC_USER32_FUNC
#define CC_USER32_FUNC
#endif

/* Not available on older SDKs */
typedef cc_uintptr _SIZE_T;

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


CC_USER32_FUNC BOOL (WINAPI *_RegisterRawInputDevices)(PCRAWINPUTDEVICE devices, UINT numDevices, UINT size);
CC_USER32_FUNC UINT (WINAPI *_GetRawInputData)(HRAWINPUT hRawInput, UINT cmd, void* data, UINT* size, UINT headerSize);
CC_USER32_FUNC BOOL (WINAPI *_SetProcessDPIAware)(void);

static void User32_LoadDynamicFuncs(void) {
	static const struct DynamicLibSym funcs[] = {
		DynamicLib_OptSym(RegisterRawInputDevices),
		DynamicLib_OptSym(GetRawInputData),
		DynamicLib_OptSym(SetProcessDPIAware)
	};
	static const cc_string user32 = String_FromConst("USER32.DLL");
	void* lib;

	DynamicLib_LoadAll(&user32, funcs, Array_Elems(funcs), &lib);
}
