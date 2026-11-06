#include "../Window.h"
#include "../Platform.h"
#include "../String_.h"
#include "../Input.h"
#include "../Funcs.h"
#include "../Bitmap.h"
#include "../Event.h"
#include "../Options.h"
#include "../Errors.h"
#include "pd_api.h"
void Cursor_SetPosition(int x, int y) { 

}
/*########################################################################################################################*
*--------------------------------------------------Public implementation--------------------------------------------------*
*#########################################################################################################################*/
void Window_PreInit(void) { }
extern PlaydateAPI* pd;
struct _DisplayData DisplayInfo;
struct cc_window WindowInfo;

#define DISP_WIDTH  LCD_COLUMNS
#define DISP_HEIGHT LCD_ROWS

void Window_Init(void) {
	DisplayInfo.Width  = DISP_WIDTH;
	DisplayInfo.Height = DISP_HEIGHT;

	DisplayInfo.ScaleX = 0.5f;
	DisplayInfo.ScaleY = 0.5f;
	
}

void Window_Free(void) {

}

static void DoCreateWindow(int width, int height) {
	Window_Main.Width    = DISP_WIDTH;
	Window_Main.Height   = DISP_HEIGHT;
	Window_Main.Focused  = true;
	
	Window_Main.Exists   = true;
	Window_Main.UIScaleX = DEFAULT_UI_SCALE_X;
	Window_Main.UIScaleY = DEFAULT_UI_SCALE_Y;
}

void Window_Create2D(int width, int height) { DoCreateWindow(width, height); }
void Window_Create3D(int width, int height) { DoCreateWindow(width, height); }

void Window_Destroy(void) { }

void Window_SetTitle(const cc_string* title) { }

void Clipboard_GetText(cc_string* value) { }

void Clipboard_SetText(const cc_string* value) { }

int Window_GetWindowState(void) {
	return WINDOW_STATE_NORMAL;
}

cc_result Window_EnterFullscreen(void) {
	return 0;
}

cc_result Window_ExitFullscreen(void) {
	return 0;
}

int Window_IsObscured(void) { return 0; }

void Window_Show(void) { }

void Window_SetSize(int width, int height) { }

void Window_RequestClose(void) {
	Event_RaiseVoid(&WindowEvents.Closing);
}

void Window_ProcessEvents(float delta) {

}

void Gamepads_PreInit(void) { }

void Gamepads_Init(void) { }

static const BindMapping pad_defaults[BIND_COUNT] = {
	[BIND_LOOK_UP]      = { CCPAD_2, CCPAD_UP },
	[BIND_LOOK_DOWN]    = { CCPAD_2, CCPAD_DOWN },
	[BIND_LOOK_LEFT]    = { CCPAD_2, CCPAD_LEFT },
	[BIND_LOOK_RIGHT]   = { CCPAD_2, CCPAD_RIGHT },
	[BIND_FORWARD]      = { CCPAD_UP,    0 },  
	[BIND_BACK]         = { CCPAD_DOWN,  0 },
	[BIND_LEFT]         = { CCPAD_LEFT,  0 },  
	[BIND_RIGHT]        = { CCPAD_RIGHT, 0 },
	[BIND_JUMP]         = { CCPAD_1, 0 },
	[BIND_INVENTORY]    = { CCPAD_R, CCPAD_UP },
	[BIND_DELETE_BLOCK] = { CCPAD_L, 0 },
	[BIND_PLACE_BLOCK]  = { CCPAD_R, 0 },
	[BIND_HOTBAR_LEFT]  = { CCPAD_L, CCPAD_LEFT }, 
	[BIND_HOTBAR_RIGHT] = { CCPAD_L, CCPAD_RIGHT }
};


#define PDMaskBit(mask) ((current&mask) ? 1.0 : 0.0)

void Gamepads_Process(float delta) { 
	int port = Gamepad_Connect(0xDA7E, pad_defaults);
	PDButtons current;
	pd->system->getButtonState(&current, NULL, NULL);
	//Camera_OnRawMovement(pd->system->getCrankChange(),0,0);
	if(!(current&kButtonB))
	{
		Gamepad_SetButton(port, CCPAD_UP, current&kButtonUp);
		Gamepad_SetButton(port, CCPAD_DOWN, current&kButtonDown);
		Gamepad_SetButton(port, CCPAD_LEFT, current&kButtonLeft);
		Gamepad_SetButton(port, CCPAD_RIGHT, current&kButtonRight);
		Gamepad_SetAxis(port, PAD_AXIS_RIGHT, pd->system->getCrankChange(), 0, delta);
		//Gamepad_SetAxis(port, PAD_AXIS_LEFT, PDMaskBit(kButtonRight)-PDMaskBit(kButtonLeft), PDMaskBit(kButtonDown)-PDMaskBit(kButtonUp), delta);
		Gamepad_SetButton(port, CCPAD_1, current&kButtonA);
	}
	else
	{
		Gamepad_SetAxis(port, PAD_AXIS_RIGHT, 0, pd->system->getCrankChange(), delta);
		//Gamepad_SetAxis(port, PAD_AXIS_LEFT, PDMaskBit(kButtonRight)-PDMaskBit(kButtonLeft), PDMaskBit(kButtonDown)-PDMaskBit(kButtonUp), delta);
		Gamepad_SetButton(port, CCPAD_R, current&kButtonRight);
		Gamepad_SetButton(port, CCPAD_L, current&kButtonLeft);
	}
}

static void ShowDialogCore(const char* title, const char* msg) {
	Platform_LogConst(title);
	Platform_LogConst(msg);
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

void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	bmp->scan0  = (BitmapCol*)Mem_Alloc(width * height, BITMAPCOLOR_SIZE, "window pixels");
	bmp->width  = width;
	bmp->height = height;
}

cc_uint8 bayer[256] = {0, 128, 32, 160, 8, 136, 40, 168, 2, 130, 34, 162, 10, 138, 42, 170,
192, 64, 224, 96, 200, 72, 232, 104, 194, 66, 226, 98, 202, 74, 234, 106,
48, 176, 16, 144, 56, 184, 24, 152, 50, 178, 18, 146, 58, 186, 26, 154,
240, 112, 208, 80, 248, 120, 216, 88, 242, 114, 210, 82, 250, 122, 218, 90,
12, 140, 44, 172, 4, 132, 36, 164, 14, 142, 46, 174, 6, 134, 38, 166,
204, 76, 236, 108, 196, 68, 228, 100, 206, 78, 238, 110, 198, 70, 230, 102,
60, 188, 28, 156, 52, 180, 20, 148, 62, 190, 30, 158, 54, 182, 22, 150,
252, 124, 220, 92, 244, 116, 212, 84, 254, 126, 222, 94, 246, 118, 214, 86,
3, 131, 35, 163, 11, 139, 43, 171, 1, 129, 33, 161, 9, 137, 41, 169,
195, 67, 227, 99, 203, 75, 235, 107, 193, 65, 225, 97, 201, 73, 233, 105,
51, 179, 19, 147, 59, 187, 27, 155, 49, 177, 17, 145, 57, 185, 25, 153,
243, 115, 211, 83, 251, 123, 219, 91, 241, 113, 209, 81, 249, 121, 217, 89,
15, 143, 47, 175, 7, 135, 39, 167, 13, 141, 45, 173, 5, 133, 37, 165,
207, 79, 239, 111, 199, 71, 231, 103, 205, 77, 237, 109, 197, 69, 229, 101,
63, 191, 31, 159, 55, 183, 23, 151, 61, 189, 29, 157, 53, 181, 21, 149,
255, 127, 223, 95, 247, 119, 215, 87, 253, 125, 221, 93, 245, 117, 213, 85};

static CC_INLINE void DrawFramebuffer(Rect2D r, struct Bitmap* bmp, char* screen) {
    for (int y = r.y; y < r.y + r.height; ++y) 
	{
        BitmapCol* row = Bitmap_GetRow(bmp, y);
		cc_uint8* bayerrow = &bayer[(y&0xf)*16];
        for (int x = r.x; x < r.x + r.width; ++x) 
		{
            // TODO optimise
            BitmapCol	col = row[x];
			cc_uint8 R = BitmapCol_R(col);
			cc_uint8 G = BitmapCol_G(col);
			cc_uint8 B = BitmapCol_B(col);
			if((x&7) == 0)
			{
				screen[(y*LCD_ROWSIZE)+(x>>3)] = 0;
			}
			
			screen[(y*LCD_ROWSIZE)+(x>>3)] |= (cc_uint8)(G >= (bayerrow[x&15]) ? 0x80 : 0)>>(x&7);
        }
    }
}

static CC_INLINE void DrawDirect(struct Bitmap* bmp, char* screen) {
	BitmapCol* src = bmp->scan0;

	for (int i = 0; i < DISP_WIDTH * DISP_HEIGHT; i++) 
	{
		BitmapCol col = src[i];
		cc_uint8 R = BitmapCol_R(col);
		cc_uint8 G = BitmapCol_G(col);
		cc_uint8 B = BitmapCol_B(col);
		if((i&7) == 0)
		{
			screen[i>>3] = 0;
		}
		screen[i>>3] |= (cc_uint8)(G > 0xC0 ? 0x80 : 0)>>(i&7);
	}
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	char* screen = pd->graphics->getFrame(); // working buffer
	//if (r.x == 0 && r.y == 0 && r.width == DISP_WIDTH && r.height == DISP_HEIGHT && bmp->width == DISP_WIDTH) {
	//	DrawDirect(bmp, screen);
	//} else {
		DrawFramebuffer(r, bmp, screen);
	//}
	pd->graphics->markUpdatedRows(0, DISP_HEIGHT);
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
}

void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) { }
void OnscreenKeyboard_SetText(const cc_string* text) { }
void OnscreenKeyboard_Close(void) { }

void Window_EnableRawMouse(void) {
	Input.RawMode = true;
}

void Window_UpdateRawMouse(void) {
	
}

void Window_DisableRawMouse(void) { 
	Input.RawMode = false;
}

