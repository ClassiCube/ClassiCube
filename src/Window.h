#ifndef CC_WINDOW_H
#define CC_WINDOW_H
#include "Core.h"
CC_BEGIN_HEADER

/* 
Abstracts interaction with a windowing system (creating window, moving cursor, etc)
Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/

struct Bitmap;
struct DynamicLibSym;
struct InputDevice;

/* The states the window can be in. */
enum WindowState  { WINDOW_STATE_NORMAL, WINDOW_STATE_FULLSCREEN, WINDOW_STATE_MINIMISED };
enum SoftKeyboard { SOFT_KEYBOARD_NONE, SOFT_KEYBOARD_RESIZE, SOFT_KEYBOARD_SHIFT, SOFT_KEYBOARD_VIRTUAL };
enum KeyboardType { KEYBOARD_TYPE_TEXT, KEYBOARD_TYPE_NUMBER, KEYBOARD_TYPE_PASSWORD, KEYBOARD_TYPE_INTEGER };
#define KEYBOARD_FLAG_SEND 0x100
/* (can't name these structs Window/Display, as that conflicts with X11's Window/Display typedef) */

/* Data for the display monitor. */
CC_VAR extern struct _DisplayData {
	/* Number of bits per pixel. (red bits + green bits + blue bits + alpha bits) */
	/* NOTE: Only 24 or 32 bits per pixel are officially supported. */
	/* Support for other values of bits per pixel is platform dependent. */
	int Depth;
	/* Scale usually based on number of physical dots per inch of the display. (horizontally and vertically) */
	/* NOTE: This may just be the display scaling set in user's settings, not actual DPI. */
	/* NOTE: Usually 1 for compatibility, even if the display's DPI is really higher. */
	/* GUI elements must be scaled by this to look correct. */
	float ScaleX, ScaleY;
	/* Position of this display. (may be non-zero in multi-monitor setup. Platform dependent) */
	int x, y;
	/* Size/Dimensions of this display in pixels. */
	int Width, Height;
	/* Whether accounting for system DPI scaling is enabled */
	cc_bool DPIScaling;
	/* Whether the soft keyboard is currently being shown */
	cc_bool ShowingSoftKeyboard;
	/* Whether the cursor is currently visible */
	cc_bool CursorVisible;
	/* Whether the framebuffer must always be entirely redrawn */
	/* NOTE: Currently only the Sega 32X requires this */
	cc_bool FullRedraw;
	/* Amount to offset content near the edges of the window by */
	/*  Mainly intended for when the game is rendered on TV displays, where */
	/*  pixels on the edges of the screen may be hidden due to overscan */
	int ContentOffsetX, ContentOffsetY;
} DisplayInfo;

/* Scales the given X coordinate from 96 dpi to current display dpi. */
static CC_INLINE int Display_ScaleX(int x) { return (int)(x * DisplayInfo.ScaleX); }
/* Scales the given Y coordinate from 96 dpi to current display dpi. */
static CC_INLINE int Display_ScaleY(int y) { return (int)(y * DisplayInfo.ScaleY); }

/* Data for a window */
struct cc_window {
	/* Readonly platform-specific handle to the window. */
	cc_pointer Handle;
	/* Size of the content area of the window. (i.e. area that can draw to) */
	/* This area does NOT include borders and titlebar surrounding the window. */
	int Width, Height;
	/* Whether the window is actually valid (i.e. not destroyed). */
	cc_bool Exists;
	/* Whether the user is interacting with the window. */
	cc_bool Focused;
	/* The type of on-screen keyboard this platform supports. (usually SOFT_KEYBOARD_NONE) */
	cc_uint8 SoftKeyboard;
	/* Whether this window is backgrounded / inactivated */
	/* (rendering is not performed when window is inactive) */
	cc_bool Inactive;
	/* Whether input should be ignored due to soft keyboard being open */
	cc_bool SoftKeyboardFocus;
	/* Whether on-screen keyboard should be instantly opened when an input field is selected */
	/* Otherwise, the on-screen keyboard is only opened when the input field is clicked */
	cc_uint8 SoftKeyboardInstant;
	/* Scale factors specifically for some in-game elements (e.g. chat) */
	/*  that vary their elements based on the window dimensions */
	float UIScaleX, UIScaleY;
};

#define DEFAULT_UI_SCALE_X (1.0f / 640)
#define DEFAULT_UI_SCALE_Y (1.0f / 480)

/* Data for the game/launcher window */
CC_VAR extern struct cc_window WindowInfo; /* Named WindowInfo for backwards compatibility */
#define Window_Main WindowInfo
/* Data for alternate game window (e.g. 3DS) */
extern struct cc_window Window_Alt;

/* Initialises necessary state before initing platform and loading options */
void Window_PreInit(void);
/* Initialises state for window, input, and display. */
void Window_Init(void);
/* Potentially frees/resets state for window, input, and display. */
void Window_Free(void);
/* Creates a window of the given size at centre of the screen. */
/* NOTE: The created window is compatible with 2D drawing */
void Window_Create2D(int width, int height);
/* Creates a window of the given size at centre of the screen. */
/* NOTE: The created window is compatible with 3D rendering */
void Window_Create3D(int width, int height);
/* Destroys the window. */
void Window_Destroy(void);

/* Sets the text of the titlebar above the window. */
CC_API void Window_SetTitle(const cc_string* title);

/* Gets the text currently on the clipboard. */
/* NOTE: On most platforms this function can be called at any time. */
/* In web backend, can only be called during INPUT_CLIPBOARD_PASTE event. */
CC_API void Clipboard_GetText(cc_string* value);
/* Sets the text currently on the clipboard. */
/* NOTE: On most platforms this function can be called at any time. */
/* In web backend, can only be called during INPUT_CLIPBOARD_COPY event. */
CC_API void Clipboard_SetText(const cc_string* value);

/* Gets the current state of the window, see WindowState enum. */
int Window_GetWindowState(void);
/* Attempts to switch the window to occupy the entire screen. */
cc_result Window_EnterFullscreen(void);
/* Attempts to restore the window to before it entered full screen. */
cc_result Window_ExitFullscreen(void);
/* Returns non-zero if the window is obscured (occluded or minimised) */
/* NOTE: Not supported by all windowing backends */
int Window_IsObscured(void);

/* Makes the window visible and focussed on screen. */
void Window_Show(void);
/* Sets the size of the internal bounds of the window in pixels. */
/* NOTE: This size excludes the bounds of borders + title */
void Window_SetSize(int width, int height);
/* Attempts to close the window. (And on some backends also destroys the window) */
/* May raise the WindowClosing and WindowClosed events. */
void Window_RequestClose(void);
/* Processes all pending window messages/events. */
void Window_ProcessEvents(float delta);

/* Initialises state for gamepad/joystick input. */
void Gamepads_Init(void);
/* Processes all pending gamepad/joystick input. */
void Gamepads_Process(float delta);

/* Sets the position of the cursor. */
/* NOTE: This should be avoided because it is unsupported on some platforms. */
void Cursor_SetPosition(int x, int y);
/* Shows a dialog box window. */
CC_API void Window_ShowDialog(const char* title, const char* msg);

#define OFD_UPLOAD_DELETE  0 /* (webclient) Deletes the uploaded file after invoking callback function */
#define OFD_UPLOAD_PERSIST 1 /* (webclient) Saves the uploded file into IndexedDB */
typedef void (*FileDialogCallback)(const cc_string* path);

struct SaveFileDialogArgs {
	const char* const* filters; /* File extensions to limit dialog to showing (e.g. ".zip", NULL) */
	const char* const* titles;  /* Descriptions to show for each file extension */
	cc_string defaultName;      /* Default filename (without extension), required by some backends */
	FileDialogCallback Callback;
};
struct OpenFileDialogArgs {
	const char* description; /* Describes the types of files supported (e.g. "Texture packs") */
	const char* const* filters; /* File extensions to limit dialog to showing (e.g. ".zip", NULL) */
	FileDialogCallback Callback;
	int uploadAction; /* Action webclient takes after invoking callback function */
	const char* uploadFolder; /* For webclient, folder to upload the file to */
};
/* Shows an 'load file' dialog window */
cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args);
/* Shows an 'save file' dialog window */
cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args);

/* Allocates a framebuffer that can be drawn/transferred to the window. */
/* NOTE: Do NOT free bmp->Scan0, use Window_FreeFramebuffer. */
/* NOTE: This MUST be called whenever the window is resized. */
void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height);
/* Transfers pixels from the allocated framebuffer to the on-screen window. */
/*   r can be used to only update a small region of pixels (may be ignored) */
/* NOTE: bmp must have come from Window_AllocFramebuffer */
void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp);
/* Frees the previously allocated framebuffer. */
void Window_FreeFramebuffer(struct Bitmap* bmp);

struct OpenKeyboardArgs { 
	const cc_string* text; 
	int type, yOffset;
	const char* placeholder; 
	cc_bool opaque, multiline; 
	struct InputDevice* device;
};
static CC_INLINE void OpenKeyboardArgs_Init(struct OpenKeyboardArgs* args, STRING_REF const cc_string* text, int type) {
	args->text    = text;
	args->type    = type;
	args->yOffset = 0;
	args->placeholder = "";
	args->opaque      = false;
	args->multiline   = false;
	args->device      = NULL;
}

/* Displays on-screen keyboard for platforms that lack physical keyboard input. */
/* NOTE: On desktop platforms, this won't do anything. */
void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args);
/* Sets the text used for keyboard input. */
/* NOTE: This is e.g. used for mobile on-screen keyboard input with the web client, */
/*  because it is backed by a HTML input, rather than true keyboard input events. */
/* As such, this is necessary to ensure the HTML input is consistent with */
/*  whatever text input widget is actually being displayed on screen. */
void OnscreenKeyboard_SetText(const cc_string* text);
/* Hides/Removes the previously displayed on-screen keyboard. */
void OnscreenKeyboard_Close(void);
/* Locks/Unlocks the landscape orientation. */
void Window_LockLandscapeOrientation(cc_bool lock);

/* Begins listening for raw input and starts raising PointerEvents.RawMoved. */
/* NOTE: Some backends only raise it when Window_UpdateRawMouse is called. */
/* Cursor will also be hidden and moved to window centre. */
void Window_EnableRawMouse(void);
/* Updates mouse state. (such as centreing cursor) */
void Window_UpdateRawMouse(void);
/* Disables listening for raw input and stops raising PointerEvents.RawMoved */
/* Cursor will also be unhidden and moved back to window centre. */
void Window_DisableRawMouse(void);

/* OpenGL contexts are heavily tied to the window, so for simplicitly are also provided here */
#if CC_GFX_BACKEND_IS_GL()
#define GLCONTEXT_DEFAULT_DEPTH 24
/* Creates an OpenGL context, then makes it the active context. */
/* NOTE: You MUST have created the window beforehand, as the GL context is attached to the window. */
void GLContext_Create(void);
/* Updates the OpenGL context after the window is resized. */
void GLContext_Update(void);
/* Attempts to restore a lost OpenGL context. */
cc_bool GLContext_TryRestore(void);
/* Destroys the OpenGL context. */
/* NOTE: This also unattaches the OpenGL context from the window. */
void GLContext_Free(void);

/* Returns the address of a function pointer for the given OpenGL function. */
/* NOTE: The implementation may still return an address for unsupported functions! */
/* You MUST check the OpenGL version and/or GL_EXTENSIONS string for actual support! */
void* GLContext_GetAddress(const char* function);

/* Swaps the front and back buffer, displaying the back buffer on screen. */
cc_bool GLContext_SwapBuffers(void);
/* Sets whether synchronisation with the monitor is enabled. */
/* NOTE: The implementation may choose to still ignore this. */
void GLContext_SetVSync(cc_bool vsync);
/* Gets OpenGL context specific graphics information. */
void GLContext_GetApiInfo(cc_string* info);
#endif

CC_END_HEADER
#endif
