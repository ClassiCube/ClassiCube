#include "../Window.h"
#include "../Platform.h"
#include "../Input.h"
#include "../Event.h"
#include "../Graphics.h"
#include "../String_.h"
#include "../Funcs.h"
#include "../Bitmap.h"
#include "../Errors.h"
#include "../ExtMath.h"
#include "../Camera.h"
#include "../VirtualDialog.h"

#include <nds.h>
#include <fat.h>

#define LAYER_CON  0
#define LAYER_KB   1


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

void Console_Clear(void) {
    for (int i = 0; i < CON_WIDTH * CON_HEIGHT; i++)
    {
        conFontBgMap[i] = ' ' - FONT_ASCII_OFFSET;
    }

    conCursorX    = 0;
    conCurrentRow = 0;
}

static void Console_NewLine(void) {
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

static void Console_PrintChar(char c) {
	if (c < ' ') return; // only ASCII supported

    if (conCursorX >= CON_WIDTH) 
        Console_NewLine();

    u16 value = conFontCurPal | (c - FONT_ASCII_OFFSET);
    conFontBgMap[conCursorX + conCurrentRow * CON_WIDTH] = value;
    conCursorX++;
}

void Console_PrintString(const char* ptr, int len) {
	if (!conFontBgMap) return;

    for (int i = 0; i < len; i++)
    {
        Console_PrintChar(ptr[i]);
    }
    Console_NewLine();
}

static void Console_LoadFont(int bgId, u16* palette) {
	conFontBgMap   = (u16*)bgGetMapPtr(bgId);
	u16* fontBgGfx = (u16*)bgGetGfxPtr(bgId);
	conFontCurPal  = 15 << 12;

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

static void Console_Init(cc_bool onSub) {
    int bgId;
	if (onSub) {
		bgId = bgInitSub(LAYER_CON, BgType_Text4bpp, BgSize_T_256x256, 22, 2);
	} else {
		bgId = bgInit(   LAYER_CON, BgType_Text4bpp, BgSize_T_256x256, 22, 2);
	}

    Console_LoadFont(bgId, onSub ? BG_PALETTE_SUB : BG_PALETTE);
    Console_Clear();
}


/*########################################################################################################################*
*------------------------------------------------------General data-------------------------------------------------------*
*#########################################################################################################################*/
struct _DisplayData DisplayInfo;
struct cc_window WindowInfo;

static void SetupVideo(cc_bool is3D) {
	if (Window_Main.Is3D == is3D) return;
	Window_Main.Is3D = is3D;

	vramSetBankA(VRAM_A_LCD);
	vramSetBankB(VRAM_B_LCD);
	vramSetBankC(VRAM_C_LCD);
	vramSetBankD(VRAM_D_LCD);
	vramSetBankH(VRAM_H_LCD);
	vramSetBankI(VRAM_I_LCD);

	if (!is3D) {
		videoSetModeSub(MODE_5_2D);
		vramSetBankC(VRAM_C_SUB_BG);

		videoSetMode(MODE_5_2D);
		vramSetBankA(VRAM_A_MAIN_BG);
	} else {
		videoSetModeSub(MODE_0_2D);
		vramSetBankH(VRAM_H_SUB_BG);
		vramSetBankI(VRAM_I_SUB_BG_0x06208000);

		videoSetMode(MODE_0_3D);
	
		vramSetBankA(VRAM_A_TEXTURE);
		vramSetBankB(VRAM_B_TEXTURE);
		vramSetBankC(VRAM_C_TEXTURE);
		vramSetBankD(VRAM_D_TEXTURE);
		vramSetBankE(VRAM_E_TEX_PALETTE);
	}

	Console_Init(is3D);
}

void Window_PreInit(void) {
	Window_Main.Is3D = 210; // So SetupVideo still runs
	SetupVideo(false);
    setBrightness(2, 0);

	DisplayInfo.Width  = SCREEN_WIDTH;
	DisplayInfo.Height = SCREEN_HEIGHT;
	DisplayInfo.ScaleX = 0.5f;
	DisplayInfo.ScaleY = 0.5f;
}

void Window_Init(void) {
	Window_Main.Width    = DisplayInfo.Width;
	Window_Main.Height   = DisplayInfo.Height;
	Window_Main.Focused  = true;
	
	Window_Main.Exists   = true;
	Window_Main.UIScaleX = DEFAULT_UI_SCALE_X;
	Window_Main.UIScaleY = DEFAULT_UI_SCALE_Y;

	Window_Main.SoftKeyboard = SOFT_KEYBOARD_RESIZE;
	Input_SetTouchMode(true);
}

void Window_Free(void) { }

void Window_Create2D(int width, int height) { 
   	SetupVideo(false);
}

void Window_Create3D(int width, int height) { 
	SetupVideo(true);
}

void Window_Destroy(void) { }

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
	
	if (DisplayInfo.ShowingSoftKeyboard) {
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
static const BindMapping defaults_nds[BIND_COUNT] = {
	[BIND_FORWARD]      = { CCPAD_UP    },  
	[BIND_BACK]         = { CCPAD_DOWN  },
	[BIND_LEFT]         = { CCPAD_LEFT  },  
	[BIND_RIGHT]        = { CCPAD_RIGHT },
	[BIND_JUMP]         = { CCPAD_1     },
	[BIND_SET_SPAWN]    = { CCPAD_START }, 
	[BIND_CHAT]         = { CCPAD_4     },
	[BIND_INVENTORY]    = { CCPAD_3     },
	[BIND_SEND_CHAT]    = { CCPAD_START },
	[BIND_PLACE_BLOCK]  = { CCPAD_L     },
	[BIND_DELETE_BLOCK] = { CCPAD_R     },
	[BIND_SPEED]        = { CCPAD_2, CCPAD_L },
	[BIND_FLY]          = { CCPAD_2, CCPAD_R },
	[BIND_NOCLIP]       = { CCPAD_2, CCPAD_3 },
	[BIND_FLY_UP]       = { CCPAD_2, CCPAD_UP },
	[BIND_FLY_DOWN]     = { CCPAD_2, CCPAD_DOWN },
	[BIND_HOTBAR_LEFT]  = { CCPAD_2, CCPAD_LEFT }, 
	[BIND_HOTBAR_RIGHT] = { CCPAD_2, CCPAD_RIGHT }
};

void Gamepads_PreInit(void) { }
void Gamepads_Init(void)    { }

void Gamepads_Process(float delta) {
	int port = Gamepad_Connect(0xD5, defaults_nds);
	int mods = keysDown() | keysHeld();
	
	Gamepad_SetButton(port, CCPAD_L, mods & KEY_L);
	Gamepad_SetButton(port, CCPAD_R, mods & KEY_R);
	
	Gamepad_SetButton(port, CCPAD_1, mods & KEY_A);
	Gamepad_SetButton(port, CCPAD_2, mods & KEY_B);
	Gamepad_SetButton(port, CCPAD_3, mods & KEY_X);
	Gamepad_SetButton(port, CCPAD_4, mods & KEY_Y);
	
	Gamepad_SetButton(port, CCPAD_START,  mods & KEY_START);
	Gamepad_SetButton(port, CCPAD_SELECT, mods & KEY_SELECT);
	
	Gamepad_SetButton(port, CCPAD_LEFT,   mods & KEY_LEFT);
	Gamepad_SetButton(port, CCPAD_RIGHT,  mods & KEY_RIGHT);
	Gamepad_SetButton(port, CCPAD_UP,     mods & KEY_UP);
	Gamepad_SetButton(port, CCPAD_DOWN,   mods & KEY_DOWN);
}


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
static int bg_id;
static u16* bg_ptr;

void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	bmp->scan0  = (BitmapCol*)Mem_Alloc(width * height, BITMAPCOLOR_SIZE, "window pixels");
	bmp->width  = width;
	bmp->height = height;

	bg_id  = bgInitSub(2, BgType_Bmp16, BgSize_B16_256x256, 2, 0);
	bg_ptr = bgGetGfxPtr(bg_id);
	SetupVideo(false);
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	cothread_yield_irq(IRQ_VBLANK);
	 
	for (int y = r.y; y < r.y + r.height; y++)
	{
		BitmapCol* src = Bitmap_GetRow(bmp, y);
		uint16_t*  dst = bg_ptr + 256 * y;
		
		for (int x = r.x; x < r.x + r.width; x++)
		{
			dst[x] = src[x];
		}
	}
	
	bgShow(bg_id);
    bgUpdate();
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
	SetupVideo(true);
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
    kbd->OnKeyPressed = OnKeyPressed;
    if (Window_Main.Is3D) videoBgDisableSub(LAYER_CON);

    keyboardInit(kbd, LAYER_KB, BgType_Text4bpp, BgSize_T_256x512,
                       14, 0, false, true);
    keyboardShow();
    bgSetPriority(4 + LAYER_KB, BG_PRIORITY_0);

    String_InitArray(kbText, kbBuffer);
	String_AppendString(&kbText, args->text);
    DisplayInfo.ShowingSoftKeyboard = true;
}

void OnscreenKeyboard_SetText(const cc_string* text) { }

void OnscreenKeyboard_Close(void) {
    keyboardHide();
	if (!DisplayInfo.ShowingSoftKeyboard) return;
    DisplayInfo.ShowingSoftKeyboard = false;

    if (Window_Main.Is3D) videoBgEnableSub(LAYER_CON);
}


/*########################################################################################################################*
*-------------------------------------------------------Misc/Other--------------------------------------------------------*
*#########################################################################################################################*/
void Window_ShowDialog(const char* title, const char* msg) {
	VirtualDialog_Show(title, msg, false);
}

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}

cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}

