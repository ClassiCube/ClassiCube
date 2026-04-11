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

#include <tos.h>
#define SCREEN_WIDTH	320
#define SCREEN_HEIGHT	200

/*########################################################################################################################*
*------------------------------------------------------General data-------------------------------------------------------*
*#########################################################################################################################*/
struct _DisplayData DisplayInfo;
struct cc_window WindowInfo;

// TODO backup palette
void Window_PreInit(void) {
	for (int i = 0; i < 16; i++) 
	{
		int R = (i & 0x1) >> 0;
		int B = (i & 0x2) >> 1;
		int G = (i & 0xC) >> 2;

		R = R ? ((R << 3) | 0x07) : 0;
		B = B ? ((B << 3) | 0x07) : 0;
		G = G ? ((G << 2) | 0x03) : 0;

		Setcolor(i, (R << 8) | (G << 4) | B);
	}
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
}

void Window_Free(void) { }

void Window_Create2D(int width, int height) {
}

void Window_Create3D(int width, int height) {
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
void Window_ProcessEvents(float delta) {

}

void Cursor_SetPosition(int x, int y) { } // Makes no sense for PSP
void Window_EnableRawMouse(void)  { Input.RawMode = true;  }
void Window_DisableRawMouse(void) { Input.RawMode = false; }

void Window_UpdateRawMouse(void)  { }


/*########################################################################################################################*
*-------------------------------------------------------Gamepads----------------------------------------------------------*
*#########################################################################################################################*/
void Gamepads_PreInit(void) { }

void Gamepads_Init(void) { }

void Gamepads_Process(float delta) { }


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	bmp->scan0  = (BitmapCol*)Mem_Alloc(width * height, BITMAPCOLOR_SIZE, "window pixels");
	bmp->width  = width;
	bmp->height = height;
}

// Each group of 16 pixels is interleaved across 4 bitplanes
struct GRWORD { UWORD plane1, plane2, plane3, plane4; };

static void Window_DrawFramebufferSlow(Rect2D r, struct Bitmap* bmp, struct GRWORD* planes) {
	Mem_Set(planes, 0, 32000);
	int idx = 0;

	for (int y = 0; y < r.height; y++) 
	{
        BitmapCol* row = Bitmap_GetRow(bmp, y);

        for (int x = 0; x < r.width; x++, idx++) 
		{
            // TODO optimise
            BitmapCol col = row[x];
			cc_uint8 R = BitmapCol_R(col) >> 7;
			cc_uint8 G = BitmapCol_G(col) >> 6;
			cc_uint8 B = BitmapCol_B(col) >> 7;

            //int pal = R | (G << 1) | (B << 2);
			int g1 = G & 0x01, g2 = (G >> 1);

			int word = idx >> 4, bit = 15 - (idx & 0x0F);

			planes[word].plane1 |= R << bit;
			planes[word].plane2 |= B << bit;
			planes[word].plane3 |= g1<< bit;
			planes[word].plane4 |= g2<< bit;
        }
    }
}

/* Extract highest red bit, highest 2 green bits, highest blue bit */
#define BC_R_HI1 (BITMAPCOLOR_R_SHIFT + BITMAPCOLOR_RGB_BITS - 1)
#define BC_G_HI1 (BITMAPCOLOR_G_SHIFT + BITMAPCOLOR_RGB_BITS - 1)
#define BC_G_HI2 (BITMAPCOLOR_G_SHIFT + BITMAPCOLOR_RGB_BITS - 2)
#define BC_B_HI1 (BITMAPCOLOR_B_SHIFT + BITMAPCOLOR_RGB_BITS - 1)

#define PLOT_PIXEL(bit) \
	src= *cur++; \
	v  = (WORD)(src >> BC_R_HI1) & 1; \
	plane1 |= v << bit;   \
	v  = (WORD)(src >> BC_B_HI1) & 1; \
	plane2 |= v << bit;   \
	v  = (WORD)(src >> BC_G_HI2) & 1; \
	plane3 |= v << bit;   \
	v  = (WORD)(src >> BC_G_HI1) & 1; \
	plane4 |= v << bit;   \

// TODO should still be done in assembly.. search up 'chunky to planar atari ST'
// Still takes ~540 milliseconds per frame (compared to 1130 milliseconds of 'slow' path in 32bpp, 1395 milliseconds of 'slow' path in 16bpp)
static void Window_DrawFramebufferBulk(struct Bitmap* bmp, struct GRWORD* planes) {
	BitmapCol* cur = bmp->scan0;
	BitmapCol* end = bmp->scan0 + SCREEN_WIDTH * SCREEN_HEIGHT;
	BitmapCol src;
	WORD v;

	while (cur < end)
	{
		WORD plane1 = 0, plane2 = 0, plane3 = 0, plane4 = 0;
		PLOT_PIXEL(15); PLOT_PIXEL(14); PLOT_PIXEL(13); PLOT_PIXEL(12);
		PLOT_PIXEL(11); PLOT_PIXEL(10); PLOT_PIXEL( 9); PLOT_PIXEL( 8);
		PLOT_PIXEL( 7); PLOT_PIXEL( 6); PLOT_PIXEL( 5); PLOT_PIXEL( 4);
		PLOT_PIXEL( 3); PLOT_PIXEL( 2); PLOT_PIXEL( 1); PLOT_PIXEL( 0);
		planes->plane1 = plane1;
		planes->plane2 = plane2;
		planes->plane3 = plane3;
		planes->plane4 = plane4;
		planes++;
    }
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	struct GRWORD* planes = (struct GRWORD*)Physbase();
	//cc_uint64 beg = Stopwatch_Measure();
	
	if (r.x == 0 && r.y == 0 && r.width == SCREEN_WIDTH && r.height == SCREEN_HEIGHT) {
		Window_DrawFramebufferBulk(bmp, planes);
	} else {
		Window_DrawFramebufferSlow(r, bmp, planes);
	}

	//cc_uint64 end = Stopwatch_Measure();
	//int E = Stopwatch_ElapsedMS(beg, end);
	//Platform_Log1("ERR: %i", &E);
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
}


/*########################################################################################################################*
*------------------------------------------------------Soft keyboard------------------------------------------------------*
*#########################################################################################################################*/
void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) {
}

void OnscreenKeyboard_SetText(const cc_string* text) { }

void OnscreenKeyboard_Close(void) {
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

