#include "Core.h"
#if CC_WIN_BACKEND == CC_WIN_BACKEND_FBDEV
/*
Windowing backend for the Snowsky Echo Disc (FiiO / Ingenic X2000), a 360x360 round-LCD
music player running embedded Linux. Pairs with the software rasteriser (SoftGPU/SoftFP).

Display: takes over the raw Linux framebuffer (/dev/fb0), single-buffered. ClassiCube
         renders into its 32bpp BGRA bitmap; Window_DrawFramebuffer converts that to the
         panel's pixel format (16bpp RGB565 or 32bpp), read from the driver at runtime.
         The panel is mounted upside-down, so output+input are rotated 180 by default.
Input  : all input is standard Linux evdev (/dev/input/eventN), matched by device name:
         - the physical buttons  ("x2000_key")  -> movement / action / jump
         - the capacitive touch  ("cst816")     -> pointer + click, and drag-to-look in 3D

NOTE: the stock UI (mq_ui/mq_player) owns /dev/fb0 and the input devices and is kept alive
      by a supervisor + hardware watchdog, so it must be stopped first (see misc/snowsky).
*/
#include "_WindowBase.h"
#include "String_.h"
#include "Funcs.h"
#include "Bitmap.h"
#include "Errors.h"
#include "Utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <linux/input.h>

/* single-screen device: the alternate window mirrors the main window */
struct cc_window Window_Alt;

static int EnvInt(const char* name, int fallback) {
	const char* v = getenv(name);
	return (v && *v) ? (int)strtol(v, NULL, 0) : fallback;
}


/*########################################################################################################################*
*--------------------------------------------------------Framebuffer------------------------------------------------------*
*#########################################################################################################################*/
static int      fb_fd = -1;
static cc_uint8* fb_mem;       /* mmap'd visible buffer */
static long     fb_memlen;
static int      fb_stride;     /* bytes per scanline (finfo.line_length) */
static int      fb_bpp;        /* bits per pixel (16 or 32, from the driver) */
static int      scr_w, scr_h;
static int      fb_rotate = 180; /* panel is mounted upside-down; 0 or 180 (env CC_FB_ROTATE) */
/* pixel bitfield layout, straight from FBIOGET_VSCREENINFO */
static int      r_off, r_len, g_off, g_len, b_off, b_len;

static void InitFramebuffer(void) {
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;

	fb_fd = open("/dev/fb0", O_RDWR);
	if (fb_fd < 0) Process_Abort2(errno, "Failed to open /dev/fb0");

	if (ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo) < 0)
		Process_Abort2(errno, "FBIOGET_FSCREENINFO failed");
	if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo) < 0)
		Process_Abort2(errno, "FBIOGET_VSCREENINFO failed");

	scr_w     = vinfo.xres;
	scr_h     = vinfo.yres;
	fb_bpp    = vinfo.bits_per_pixel;
	fb_stride = finfo.line_length;
	fb_memlen = finfo.smem_len;

	r_off = vinfo.red.offset;    r_len = vinfo.red.length;
	g_off = vinfo.green.offset;  g_len = vinfo.green.length;
	b_off = vinfo.blue.offset;   b_len = vinfo.blue.length;

	fb_mem = (cc_uint8*)mmap(NULL, fb_memlen, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
	if (fb_mem == MAP_FAILED) Process_Abort2(errno, "Failed to mmap /dev/fb0");

	Platform_Log4("FB %ix%i, %i bpp, stride %i", &scr_w, &scr_h, &fb_bpp, &fb_stride);
}

void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	bmp->scan0  = (BitmapCol*)Mem_Alloc(width * height, BITMAPCOLOR_SIZE, "window pixels");
	bmp->width  = width;
	bmp->height = height;
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
}

/* Packs an 8-bit-per-channel colour into the panel's native pixel using its bitfields */
static CC_INLINE cc_uint32 PackPixel(BitmapCol c) {
	cc_uint32 R = BitmapCol_R(c) >> (8 - r_len);
	cc_uint32 G = BitmapCol_G(c) >> (8 - g_len);
	cc_uint32 B = BitmapCol_B(c) >> (8 - b_len);
	return (R << r_off) | (G << g_off) | (B << b_off);
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	int x0 = r.x, y0 = r.y;
	int x1 = r.x + r.width, y1 = r.y + r.height;
	int x, y;

	if (x0 < 0) x0 = 0;
	if (y0 < 0) y0 = 0;
	if (x1 > scr_w)       x1 = scr_w;
	if (x1 > bmp->width)  x1 = bmp->width;
	if (y1 > scr_h)       y1 = scr_h;
	if (y1 > bmp->height) y1 = bmp->height;

	for (y = y0; y < y1; y++) {
		int dy = (fb_rotate == 180) ? (scr_h - 1 - y) : y;
		BitmapCol* src = Bitmap_GetRow(bmp, y);
		if (fb_bpp == 32) {
			cc_uint32* dst = (cc_uint32*)(fb_mem + (cc_uintptr)dy * fb_stride);
			if (fb_rotate == 180) { for (x = x0; x < x1; x++) dst[scr_w - 1 - x] = PackPixel(src[x]); }
			else                  { for (x = x0; x < x1; x++) dst[x]             = PackPixel(src[x]); }
		} else if (fb_bpp == 16) {
			cc_uint16* dst = (cc_uint16*)(fb_mem + (cc_uintptr)dy * fb_stride);
			if (fb_rotate == 180) { for (x = x0; x < x1; x++) dst[scr_w - 1 - x] = (cc_uint16)PackPixel(src[x]); }
			else                  { for (x = x0; x < x1; x++) dst[x]             = (cc_uint16)PackPixel(src[x]); }
		}
	}
}


/*########################################################################################################################*
*------------------------------------------------------Input (evdev)------------------------------------------------------*
*#########################################################################################################################*/
/* Opens the /dev/input/eventN device whose name contains `match` (non-blocking, grabbed). */
static int OpenEvdev(const char* match) {
	char path[32], name[128];
	int i, fd;

	for (i = 0; i < 32; i++) {
		snprintf(path, sizeof(path), "/dev/input/event%i", i);
		fd = open(path, O_RDONLY | O_NONBLOCK);
		if (fd < 0) continue;

		name[0] = '\0';
		if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) >= 0 && strstr(name, match)) {
			ioctl(fd, EVIOCGRAB, (void*)1); /* take input from the (now stopped) stock UI */
			Platform_Log2("input: %c (event%i)", name, &i);
			return fd;
		}
		close(fd);
	}
	Platform_Log1("input: '%c' not found", match);
	return -1;
}

/*------------------------------------------------------ Buttons -----------------------------------------------------------*/
/* The physical buttons are exposed by the "x2000_key" evdev device as ordinary EV_KEY events
   (value 1=press, 2=repeat, 0=release). Codes are vendor specific (found by reverse
   engineering mq_player); set CC_KEY_DEBUG=1 to log codes when porting to another unit. */
/* evdev codes reported by the x2000_key device (prefixed to avoid clashing with the KEY_*
   names in <linux/input-event-codes.h>). Each physical button reports two codes except
   Play/Pause. The volume buttons auto-repeat while held; Power is a momentary tap. */
#define SK_FWD_A     263  /* Vol+       -> move forward (W)         */
#define SK_FWD_B     251
#define SK_BACK_A    262  /* Vol-       -> move back    (S)         */
#define SK_BACK_B    267
#define SK_PLAY      250  /* Play/Pause -> action      (left click) */
#define SK_JUMP_A    265  /* Power      -> jump        (space)      */
#define SK_JUMP_B    259
#define JUMP_HOLD 0.15f    /* how long to latch a jump for a momentary Power tap */

static int   key_fd  = -1;     /* x2000_key evdev node */
static int   gate_fd = -1;     /* /dev/key_ioctl: opened as a prerequisite, never read */
static int   key_debug;
static float jump_hold;        /* remaining latched-jump time; <=0 = released */
static cc_uint8 btn_down[288]; /* evdev code -> current pressed state (codes are 250..267) */

static void InitButtons(void) {
	key_debug = EnvInt("CC_KEY_DEBUG", 0);
	/* The stock UI opens this char device (O_RDWR) and holds it; the kernel appears to gate
	   button reporting on it, so open + hold it too. The events come from evdev, not here. */
	gate_fd = open("/dev/key_ioctl", O_RDWR);
	key_fd  = OpenEvdev("x2000_key");
}

static void ProcessButtons(float delta) {
	struct input_event ev;
	int down;
	if (key_fd < 0) return;

	while (read(key_fd, &ev, sizeof(ev)) == sizeof(ev)) {
		if (ev.type != EV_KEY) continue;
		down = ev.value != 0;   /* press(1) / repeat(2) -> down, release(0) -> up */
		if (key_debug) { int code = ev.code; Platform_Log2("key code=%i down=%i", &code, &down); }
		if (ev.code < 288) btn_down[ev.code] = (cc_uint8)down;

		switch (ev.code) {
		case SK_PLAY:    Input_SetNonRepeatable(CCMOUSE_L, down); break;  /* action = left click */
		/* Power is momentary (press+release in one frame, no auto-repeat), so latch the jump key
		   held for a moment on press, otherwise a tap is over before a physics tick sees it. */
		case SK_JUMP_A:
		case SK_JUMP_B:  if (down) { Input_SetPressed(CCKEY_SPACE); jump_hold = JUMP_HOLD; } break;
		}
	}

	/* Vol+/Vol- each report two codes and auto-repeat while held; OR their states so the
	   movement key stays pressed for the whole hold and releases cleanly (no coast/stutter). */
	Input_Set(CCKEY_W, btn_down[SK_FWD_A]  | btn_down[SK_FWD_B]);
	Input_Set(CCKEY_S, btn_down[SK_BACK_A] | btn_down[SK_BACK_B]);

	if (jump_hold > 0.0f) { jump_hold -= delta; if (jump_hold <= 0.0f) Input_SetReleased(CCKEY_SPACE); }
}

/*------------------------------------------------------ Touchscreen ------------------------------------------------------*/
static int ts_fd = -1;
static int ts_minx, ts_maxx, ts_miny, ts_maxy;
static int rawX, rawY;            /* latest raw ABS values          */
static int touchX, touchY;        /* mapped to screen pixels        */
static int touching;              /* finger currently down          */
static int lookHasPrev;           /* have a previous point for look */
static int lookPrevX, lookPrevY;

static void ReadAbsRange(int axis, int* lo, int* hi) {
	struct input_absinfo abs;
	*lo = 0; *hi = 0;
	if (ioctl(ts_fd, EVIOCGABS(axis), &abs) >= 0) { *lo = abs.minimum; *hi = abs.maximum; }
}

static void InitTouch(void) {
	ts_fd = OpenEvdev("cst816");
	if (ts_fd < 0) return;

	ReadAbsRange(ABS_X, &ts_minx, &ts_maxx);
	ReadAbsRange(ABS_Y, &ts_miny, &ts_maxy);
	if (ts_maxx <= ts_minx) { ts_minx = 0; ts_maxx = scr_w - 1; }
	if (ts_maxy <= ts_miny) { ts_miny = 0; ts_maxy = scr_h - 1; }
}

static int MapAxis(int raw, int lo, int hi, int size) {
	int v;
	if (hi <= lo) return 0;
	v = (raw - lo) * (size - 1) / (hi - lo);
	if (v < 0) v = 0;
	if (v >= size) v = size - 1;
	return v;
}

static void TouchCommit(void) {
	touchX = MapAxis(rawX, ts_minx, ts_maxx, scr_w);
	touchY = MapAxis(rawY, ts_miny, ts_maxy, scr_h);
	if (fb_rotate == 180) { touchX = scr_w - 1 - touchX; touchY = scr_h - 1 - touchY; }

	if (Input.RawMode) {
		/* in-game: drag to look. Feed relative motion, no jump on initial contact. */
		if (touching && lookHasPrev) {
			int dx = touchX - lookPrevX, dy = touchY - lookPrevY;
			if (dx || dy) Event_RaiseRawMove(&PointerEvents.RawMoved, (float)dx, (float)dy);
		}
		lookPrevX = touchX; lookPrevY = touchY;
		lookHasPrev = touching;
	} else {
		/* menus/launcher: emulate a mouse pointer + left click */
		Pointer_SetPosition(0, touchX, touchY);
	}
}

static void ProcessTouch(void) {
	struct input_event ev;
	if (ts_fd < 0) return;

	while (read(ts_fd, &ev, sizeof(ev)) == sizeof(ev)) {
		switch (ev.type) {
		case EV_ABS:
			if (ev.code == ABS_X || ev.code == ABS_MT_POSITION_X) rawX = ev.value;
			if (ev.code == ABS_Y || ev.code == ABS_MT_POSITION_Y) rawY = ev.value;
			break;
		case EV_KEY:
			if (ev.code == BTN_TOUCH) {
				touching = ev.value;
				if (!touching) lookHasPrev = false;
				if (!Input.RawMode) Input_SetNonRepeatable(CCMOUSE_L, touching);
			}
			break;
		case EV_SYN:
			if (ev.code == SYN_REPORT) TouchCommit();
			break;
		}
	}
}


/*########################################################################################################################*
*-------------------------------------------------------Window common-----------------------------------------------------*
*#########################################################################################################################*/
void Window_PreInit(void) {
	DisplayInfo.CursorVisible = true;
}

void Window_Init(void) {
	Input.Sources = INPUT_SOURCE_NORMAL;
	fb_rotate = EnvInt("CC_FB_ROTATE", 180);
	InitFramebuffer();

	DisplayInfo.Width  = scr_w;
	DisplayInfo.Height = scr_h;
	DisplayInfo.Depth  = 32;   /* engine always works in a 32bpp bitmap; panel conv is ours */
	/* Small round panel: shrink 2D UI so menus/buttons fit. */
	DisplayInfo.ScaleX = 0.5f;
	DisplayInfo.ScaleY = 0.5f;

	Window_Main.Width    = scr_w;
	Window_Main.Height   = scr_h;
	Window_Main.Exists   = true;
	Window_Main.Focused  = true;
	Window_Main.UIScaleX = DEFAULT_UI_SCALE_X;
	Window_Main.UIScaleY = DEFAULT_UI_SCALE_Y;
	Window_Alt = Window_Main;

	InitButtons();
	InitTouch();
	Platform_Flags |= PLAT_FLAG_SINGLE_PROCESS;
}

void Window_Free(void) {
	if (fb_mem && fb_mem != MAP_FAILED) munmap(fb_mem, fb_memlen);
	if (fb_fd   >= 0) close(fb_fd);
	if (ts_fd   >= 0) { ioctl(ts_fd,  EVIOCGRAB, (void*)0); close(ts_fd); }
	if (key_fd  >= 0) { ioctl(key_fd, EVIOCGRAB, (void*)0); close(key_fd); }
	if (gate_fd >= 0) close(gate_fd);
	fb_fd = ts_fd = key_fd = gate_fd = -1;
}

static void DoCreateWindow(int width, int height) {
	Window_Main.Exists   = true;
	Window_Main.Focused  = true;
	Window_Main.UIScaleX = DEFAULT_UI_SCALE_X;
	Window_Main.UIScaleY = DEFAULT_UI_SCALE_Y;
}
void Window_Create2D(int width, int height) { DoCreateWindow(width, height); }
void Window_Create3D(int width, int height) { DoCreateWindow(width, height); }

void Window_Destroy(void) { }

void Window_SetTitle(const cc_string* title) { }
void Clipboard_GetText(cc_string* value)     { }
void Clipboard_SetText(const cc_string* value) { }

int Window_GetWindowState(void)     { return WINDOW_STATE_FULLSCREEN; }
cc_result Window_EnterFullscreen(void) { return 0; }
cc_result Window_ExitFullscreen(void)  { return 0; }
int Window_IsObscured(void)         { return 0; }

void Window_Show(void)     { }
void Window_SetSize(int width, int height) { }
void Window_RequestClose(void) {
	Window_Main.Exists = false;
	Event_RaiseVoid(&WindowEvents.Closing);
}

void Window_ProcessEvents(float delta) {
	ProcessButtons(delta);
	ProcessTouch();
}

void Gamepads_PreInit(void) { }
void Gamepads_Init(void)    { }
void Gamepads_Process(float delta) { }

static void Cursor_GetRawPos(int* x, int* y) { *x = touchX; *y = touchY; }
void Cursor_SetPosition(int x, int y) { }
static void Cursor_DoSetVisible(cc_bool visible) { }

static void ShowDialogCore(const char* title, const char* msg) {
	Platform_LogConst(title);
	Platform_LogConst(msg);
}

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args) { return ERR_NOT_SUPPORTED; }
cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args) { return ERR_NOT_SUPPORTED; }

void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) { }
void OnscreenKeyboard_SetText(const cc_string* text)      { }
void OnscreenKeyboard_Close(void) { }

/* Touch drag-to-look raises PointerEvents.RawMoved directly, so no cursor centring is needed. */
void Window_EnableRawMouse(void)  { Input.RawMode = true;  lookHasPrev = false; }
void Window_UpdateRawMouse(void)  { }
void Window_DisableRawMouse(void) { Input.RawMode = false; lookHasPrev = false; }
#endif
