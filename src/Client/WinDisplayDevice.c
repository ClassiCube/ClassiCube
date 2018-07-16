#include "DisplayDevice.h"
#if CC_BUILD_WIN
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#define _WIN32_WINNT 0x0500
#include <windows.h>

void DisplayDevice_Init(void) {
	/* Get available video adapters and enumerate all monitors */
	UInt32 displayNum = 0;
	DISPLAY_DEVICEW display = { 0 }; display.cb = sizeof(DISPLAY_DEVICEW);

	while (EnumDisplayDevicesW(NULL, displayNum, &display, 0)) {
		displayNum++;
		if ((display.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) == 0) continue;
		DEVMODEW mode = { 0 }; mode.dmSize = sizeof(DEVMODEW);

		/* The second function should only be executed when the first one fails (e.g. when the monitor is disabled) */
		if (EnumDisplaySettingsW(display.DeviceName, ENUM_CURRENT_SETTINGS, &mode) ||
			EnumDisplaySettingsW(display.DeviceName, ENUM_REGISTRY_SETTINGS, &mode)) {
		} else {
			mode.dmBitsPerPel = 0;
		}

		/* This device has no valid resolution, ignore it */
		if (mode.dmBitsPerPel == 0) continue;
		bool isPrimary = (display.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) != 0;
		if (!isPrimary) continue;

		struct DisplayDevice device = { 0 };
		device.Bounds.Width = mode.dmPelsWidth;
		device.Bounds.Height = mode.dmPelsHeight;
		device.BitsPerPixel = mode.dmBitsPerPel;
		device.RefreshRate = mode.dmDisplayFrequency;
		DisplayDevice_Default = device;
	}
}
#endif