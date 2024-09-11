#include "Core.h"
#if defined CC_BUILD_NDS
#include "Window.h"
#include "Platform.h"
#include "Input.h"
#include "Event.h"
#include "Graphics.h"
#include "String.h"
#include "Funcs.h"
#include "Bitmap.h"
#include "Errors.h"
#include "ExtMath.h"
#include "Camera.h"
#include <nds/arm9/background.h>
#include <nds/arm9/input.h>
#include <nds/arm9/keyboard.h>
#include <nds/interrupts.h>
#include <nds/system.h>
#include <fat.h>


/*########################################################################################################################*
*----------------------------------------------------Onscreen console-----------------------------------------------------*
*#########################################################################################################################*/
// A majorly cutdown version of the Console included in libnds
#define CON_WIDTH  32
#define CON_HEIGHT 24

extern u8 default_fontTiles[];
#define FONT_NUM_CHARACTERS 96

#define FONT_ASCII_OFFSET 32
static u16* conFontBgMap;
static int  conFontCurPal;
static int  conCursorX, conCurrentRow;

static void consoleClear(void) {
    for (int i = 0; i < CON_WIDTH * CON_HEIGHT; i++)
    {
        conFontBgMap[i] = ' ' - FONT_ASCII_OFFSET;
    }

    conCursorX    = 0;
    conCurrentRow = 0;
}

static void consoleNewLine(void) {
    conCursorX = 0;
    conCurrentRow++;
    if (conCurrentRow < CON_HEIGHT) return;

    // Shift entire screen upwards by one row
    conCurrentRow--;

    for (int y = 0; y < CON_HEIGHT - 1; y++)
    {
        for (int x = 0; x < CON_WIDTH; x++)
        {
            int src = x + (y + 1) * CON_WIDTH;
            int dst = x + (y    ) * CON_WIDTH;
            conFontBgMap[dst] = conFontBgMap[src];
        }
    }

    for (int x = 0; x < CON_WIDTH; x++)
    {
        int index = x + (CON_HEIGHT - 1) * CON_WIDTH;
        conFontBgMap[index] = ' ' - FONT_ASCII_OFFSET;
    }
}

static void consolePrintChar(char c) {
	if (c < ' ') return; // only ASCII supported

    if (conCursorX >= CON_WIDTH) 
        consoleNewLine();

    u16 value = conFontCurPal | (c - FONT_ASCII_OFFSET);
    conFontBgMap[conCursorX + conCurrentRow * CON_WIDTH] = value;
    conCursorX++;
}

void consolePrintString(const char* ptr, int len) {
	if (!conFontBgMap) return;

    for (int i = 0; i < len; i++)
    {
        consolePrintChar(ptr[i]);
    }
    consoleNewLine();
}

static void consoleLoadFont(u16* fontBgGfx) {
    u16* palette  = BG_PALETTE_SUB;
    conFontCurPal = 15 << 12;

    for (int i = 0; i < FONT_NUM_CHARACTERS * 8; i++)
    {
        u8 row  = default_fontTiles[i];
        u32 gfx = 0;
        if (row & 0x01) gfx |= 0x0000000F;
        if (row & 0x02) gfx |= 0x000000F0;
        if (row & 0x04) gfx |= 0x00000F00;
        if (row & 0x08) gfx |= 0x0000F000;
        if (row & 0x10) gfx |= 0x000F0000;
        if (row & 0x20) gfx |= 0x00F00000;
        if (row & 0x40) gfx |= 0x0F000000;
        if (row & 0x80) gfx |= 0xF0000000;
        ((u32 *)fontBgGfx)[i] = gfx;
    }

    palette[16 * 16 - 1] = RGB15(31, 31, 31);
    palette[0]           = RGB15( 0,  0,  0);
}

static void consoleInit(void) {
    int bgId = bgInitSub(0, BgType_Text4bpp, BgSize_T_256x256, 22, 2);
    conFontBgMap = (u16*)bgGetMapPtr(bgId);

    consoleLoadFont((u16*)bgGetGfxPtr(bgId));
    consoleClear();
}


/*########################################################################################################################*
*------------------------------------------------------General data-------------------------------------------------------*
*#########################################################################################################################*/
static cc_bool launcherMode;
cc_bool keyboardOpen;
static int bg_id;
static u16* bg_ptr;

struct _DisplayData DisplayInfo;
struct _WindowData WindowInfo;

void Window_PreInit(void) {
	videoSetModeSub(MODE_0_2D);
    vramSetBankH(VRAM_H_SUB_BG);
	vramSetBankI(VRAM_I_SUB_BG_0x06208000);
    setBrightness(2, 0);
	consoleInit();
}

void Window_Init(void) {  
	DisplayInfo.Width  = SCREEN_WIDTH;
	DisplayInfo.Height = SCREEN_HEIGHT;
	DisplayInfo.ScaleX = 0.5f;
	DisplayInfo.ScaleY = 0.5f;
	
	Window_Main.Width    = DisplayInfo.Width;
	Window_Main.Height   = DisplayInfo.Height;
	Window_Main.Focused  = true;
	
	Window_Main.Exists   = true;
	Window_Main.UIScaleX = DEFAULT_UI_SCALE_X;
	Window_Main.UIScaleY = DEFAULT_UI_SCALE_Y;

	Window_Main.SoftKeyboard = SOFT_KEYBOARD_RESIZE;
	Input_SetTouchMode(true);
	Input.Sources = INPUT_SOURCE_GAMEPAD;
}

void Window_Free(void) { }

void Window_Create2D(int width, int height) { 
    launcherMode = true;
	videoSetMode(MODE_5_2D);
	vramSetBankA(VRAM_A_MAIN_BG);
	
	bg_id  = bgInit(2, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
	bg_ptr = bgGetGfxPtr(bg_id);
}

void Window_Create3D(int width, int height) { 
    launcherMode = false;
	videoSetMode(MODE_0_3D);
}

void Window_SetTitle(const cc_string* title) { }
void Clipboard_GetText(cc_string* value) { }
void Clipboard_SetText(const cc_string* value) { }

int Window_GetWindowState(void) { return WINDOW_STATE_FULLSCREEN; }
cc_result Window_EnterFullscreen(void) { return 0; }
cc_result Window_ExitFullscreen(void)  { return 0; }
int Window_IsObscured(void)            { return 0; }

void Window_Show(void) { }
void Window_SetSize(int width, int height) { }

void Window_RequestClose(void) {
	Event_RaiseVoid(&WindowEvents.Closing);
}


/*########################################################################################################################*
*----------------------------------------------------Input processing-----------------------------------------------------*
*#########################################################################################################################*/
static void ProcessTouchInput(int mods) {
	touchPosition touch;
	touchRead(&touch);
	Camera.Sensitivity = 100; // TODO not hardcode this
	
	if (mods & KEY_TOUCH) {
		Input_AddTouch(0,    touch.px,      touch.py);
	} else if (keysUp() & KEY_TOUCH) {
		Input_RemoveTouch(0, Pointers[0].x, Pointers[0].y);
	}
}

void Window_ProcessEvents(float delta) {
	scanKeys();	
	
	if (keyboardOpen) {
		keyboardUpdate();
	} else {
		ProcessTouchInput(keysDown() | keysHeld());
	}
}

void Cursor_SetPosition(int x, int y) { } // Makes no sense for PSP
void Window_EnableRawMouse(void)  { Input.RawMode = true;  }
void Window_DisableRawMouse(void) { Input.RawMode = false; }

void Window_UpdateRawMouse(void)  { }


/*########################################################################################################################*
*-------------------------------------------------------Gamepads----------------------------------------------------------*
*#########################################################################################################################*/
void Window_ProcessGamepads(float delta) {
	int mods = keysDown() | keysHeld();
	
	Gamepad_SetButton(0, CCPAD_L, mods & KEY_L);
	Gamepad_SetButton(0, CCPAD_R, mods & KEY_R);
	
	Gamepad_SetButton(0, CCPAD_A, mods & KEY_A);
	Gamepad_SetButton(0, CCPAD_B, mods & KEY_B);
	Gamepad_SetButton(0, CCPAD_X, mods & KEY_X);
	Gamepad_SetButton(0, CCPAD_Y, mods & KEY_Y);
	
	Gamepad_SetButton(0, CCPAD_START,  mods & KEY_START);
	Gamepad_SetButton(0, CCPAD_SELECT, mods & KEY_SELECT);
	
	Gamepad_SetButton(0, CCPAD_LEFT,   mods & KEY_LEFT);
	Gamepad_SetButton(0, CCPAD_RIGHT,  mods & KEY_RIGHT);
	Gamepad_SetButton(0, CCPAD_UP,     mods & KEY_UP);
	Gamepad_SetButton(0, CCPAD_DOWN,   mods & KEY_DOWN);
}


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	bmp->scan0  = (BitmapCol*)Mem_Alloc(width * height, 4, "window pixels");
	bmp->width  = width;
	bmp->height = height;
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	swiWaitForVBlank();
	 
	for (int y = r.y; y < r.y + r.height; y++)
	{
		BitmapCol* src = Bitmap_GetRow(bmp, y);
		uint16_t*  dst = bg_ptr + 256 * y;
		
		for (int x = r.x; x < r.x + r.width; x++)
		{
			BitmapCol color = src[x];
			// 888 to 565 (discard least significant bits)
			// quoting libDNS: < Bitmap background with 16 bit color values of the form aBBBBBGGGGGRRRRR (if 'a' is not set, the pixel will be transparent)
			dst[x] = 0x8000 | ((BitmapCol_B(color) & 0xF8) << 7) | ((BitmapCol_G(color) & 0xF8) << 2) | (BitmapCol_R(color) >> 3);
		}
	}
	
	bgShow(bg_id);
    bgUpdate();
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
}


/*########################################################################################################################*
*------------------------------------------------------Soft keyboard------------------------------------------------------*
*#########################################################################################################################*/
static char kbBuffer[NATIVE_STR_LEN + 1];
static cc_string kbText;

static void OnKeyPressed(int key) {
    if (key == 0) {
        OnscreenKeyboard_Close();
    } else if (key == DVK_ENTER) {
        OnscreenKeyboard_Close();
        Input_SetPressed(CCKEY_ENTER);
        Input_SetReleased(CCKEY_ENTER);
    } else if (key == DVK_BACKSPACE) {
        if (kbText.length) kbText.length--;
        Event_RaiseString(&InputEvents.TextChanged, &kbText);
    } else if (key > 0) {
        String_Append(&kbText, key);
        Event_RaiseString(&InputEvents.TextChanged, &kbText);
    }
}

void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) { 
    Keyboard* kbd = keyboardGetDefault();
    videoBgDisableSub(0); // hide console

    keyboardInit(kbd, 3, BgType_Text4bpp, BgSize_T_256x512,
                       14, 0, false, true);
    keyboardShow();

    kbd->OnKeyPressed = OnKeyPressed;
    String_InitArray(kbText, kbBuffer);
	String_AppendString(&kbText, args->text);
    keyboardOpen = true;
}

void OnscreenKeyboard_SetText(const cc_string* text) { }

void OnscreenKeyboard_Draw2D(Rect2D* r, struct Bitmap* bmp) { }
void OnscreenKeyboard_Draw3D(void) { }

void OnscreenKeyboard_Close(void) {
    keyboardHide();
	if (!keyboardOpen) return;
    keyboardOpen = false;

    videoBgEnableSub(0); // show console
}


/*########################################################################################################################*
*-------------------------------------------------------Misc/Other--------------------------------------------------------*
*#########################################################################################################################*/
void Window_ShowDialog(const char* title, const char* msg) {
	/* TODO implement */
	Platform_LogConst(title);
	Platform_LogConst(msg);
}

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}

cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}
#endif
