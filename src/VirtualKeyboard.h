#include "Core.h"
#include "Funcs.h"
#include "Drawer2D.h"
#include "Event.h"
#include "ExtMath.h"
#include "String.h"
#include "Input.h"
#include "Utils.h"
#include "LBackend.h"
#include "Window.h"
#include "Graphics.h"
#include "Game.h"

static cc_bool kb_inited, kb_shift, kb_needsHook;
static struct FontDesc kb_font;
static int kb_curX, kb_curY;
static float kb_padXAcc, kb_padYAcc;
static char kb_buffer[512];
static cc_string kb_str = String_FromArray(kb_buffer);
static void (*KB_MarkDirty)(void);
static int kb_yOffset;
static cc_bool kb_clicking;

#define KB_TILE_SIZE 32
static int kb_tileWidth  = KB_TILE_SIZE;
static int kb_tileHeight = KB_TILE_SIZE;

#define KB_B_CAPS  0x10
#define KB_B_SHIFT 0x20
#define KB_B_SPACE 0x30
#define KB_B_CLOSE 0x40
#define KB_B_BACK  0x50
#define KB_B_ENTER 0x60

#define KB_GetBehaviour(i) (kb->behaviour[i] & 0xF0)
#define KB_GetCellWidth(i) (kb->behaviour[i] & 0x0F)
#define KB_IsInvisible(i)  (kb->behaviour[i] == 0)

struct KBLayout {
	int numRows, cellsPerRow, rowWidth;
	const char** lower;
	const char** upper;
	const char** table;
	const cc_uint8* behaviour;
};
static struct KBLayout* kb;


static const char* kb_normal_lower[] =
{
	"1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "Backspace",
	"q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "(", ")", "&    ",
	"a", "s", "d", "f", "g", "h", "j", "k", "l", "?", ";", "'", "Enter",
	"z", "x", "c", "v", "b", "n", "m", ",", ".","\\", "!", "@", "/    ",
	"Caps",0,0,0, "Shift",0,0,0, "Space",0,0,0, "Close"
};
static const char* kb_normal_upper[] =
{
	"1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "_", "+", "Backspace",
	"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "[", "]", "&   ",
	"A", "S", "D", "F", "G", "H", "J", "K", "L", "?", ":", "\"", "Enter",
	"Z", "X", "C", "V", "B", "N", "M", "<", ">", "*", "%", "#", "/    ",
	"Caps",0,0,0, "Shift",0,0,0, "Space",0,0,0, "Close"
};
static const cc_uint8 kb_normal_behaviour[] =
{
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, KB_B_BACK | 4,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, KB_B_ENTER | 4,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4,
	KB_B_CAPS | 4,0,0,0, KB_B_SHIFT | 4,0,0,0, KB_B_SPACE | 4,0,0,0, KB_B_CLOSE | 4
};

static struct KBLayout normal_layout = {
	5, 13, 16, // 12 normal cells, 1 four wide cell
	kb_normal_lower, kb_normal_upper, kb_normal_lower,
	kb_normal_behaviour
};


static const char* kb_numpad[] =
{
	"1", "2", "3", "Backspace",
	"4", "5", "6", "Enter",
	"7", "8", "9", "Space",
	"-", "0", ".", "Close",
};
static const cc_uint8 kb_numpad_behaviour[] =
{
	1, 1, 1, KB_B_BACK  | 4,
	1, 1, 1, KB_B_ENTER | 4,
	1, 1, 1, KB_B_SPACE | 4,
	1, 1, 1, KB_B_CLOSE | 4
};

static struct KBLayout numpad_layout = {
	4, 4, 7, // 3 normal cells, 1 four wide cell
	kb_numpad, kb_numpad, kb_numpad,
	kb_numpad_behaviour
};


extern void LWidget_DrawBorder(struct Context2D* ctx, BitmapCol color, int borderX, int borderY,
								int x, int y, int width, int height);

static void VirtualKeyboard_Init(void) {
	if (kb_inited) return;
	kb_inited = true;
	Font_Make(&kb_font, 16, FONT_FLAGS_NONE);
}

static int VirtualKeyboard_Width(void) {
	return kb->rowWidth * kb_tileWidth;
}

static int VirtualKeyboard_Height(void) {
	return kb->numRows  * kb_tileHeight;
}

static int VirtualKeyboard_GetSelected(void) {
	if (kb_curX < 0) return -1;
	int maxCells = kb->cellsPerRow * kb->numRows;

	int idx = kb_curX + kb->cellsPerRow * kb_curY;
	Math_Clamp(idx, 0, maxCells - 1);
	
	// Skip over invisible cells
	while (KB_IsInvisible(idx)) idx--;
	return idx;
}

static void VirtualKeyboard_Close(void);
static void VirtualKeyboard_Hook(void);


#define KB_NORMAL_COLOR     BitmapCol_Make(0x7F, 0x7F, 0x7F, 0x90)
#define KB_SELECTED_COLOR   BitmapCol_Make(0xAF, 0xAF, 0xAF, 0x90)
#define KB_BACKGROUND_COLOR BitmapCol_Make(0xFF, 0xFF, 0xFF, 0x90)

static void VirtualKeyboard_Draw(struct Context2D* ctx) {
	struct DrawTextArgs args;
	cc_string str;
	int row, cell;
	int i = 0, x, y, w, h, dx, dy;
	int selected = VirtualKeyboard_GetSelected();
	
	Drawer2D.Colors['f'] = Drawer2D.Colors['0'];
	if (kb_needsHook) VirtualKeyboard_Hook();

	for (row = 0, y = 0; row < kb->numRows; row++)
	{
		for (cell = 0, x = 0; cell < kb->cellsPerRow; cell++, i++)
		{
			if (KB_IsInvisible(i)) continue;

			str = String_FromReadonly(kb->table[i]);
			DrawTextArgs_Make(&args, &str, &kb_font, false);
			w = kb_tileWidth * KB_GetCellWidth(i);
			h = kb_tileHeight;

			Gradient_Noise(ctx, i == selected ? KB_SELECTED_COLOR : KB_NORMAL_COLOR, 4, x, y, w, h);
			LWidget_DrawBorder(ctx, KB_BACKGROUND_COLOR, 1, 1, x, y, w, h);

			dx = (w - Drawer2D_TextWidth (&args)) / 2;
			dy = (h - Drawer2D_TextHeight(&args)) / 2;
			Context2D_DrawText(ctx, &args, x + dx, y + dy);

			x += w;
		}
		y += kb_tileHeight;
	}
	
	Drawer2D.Colors['f'] = Drawer2D.Colors['F'];
}

static void VirtualKeyboard_CalcPosition(int* x, int* y, int width, int height) {
	/* Draw virtual keyboard at centre of window bottom */
	*y = height - 1 - VirtualKeyboard_Height() - kb_yOffset;
	if (*y < 0) *y = 0;

	*x = (width - VirtualKeyboard_Width()) / 2;
	if (*x < 0) *x = 0;
}

static int VirtualKeyboard_WindowWidth(void) {
#ifdef CC_BUILD_DUALSCREEN
	return launcherMode ? Window_Main.Width : Window_Alt.Width;
#else
	return Window_Main.Width;
#endif
}

static int VirtualKeyboard_WindowHeight(void) {
#ifdef CC_BUILD_DUALSCREEN
	return launcherMode ? Window_Main.Height : Window_Alt.Height;
#else
	return Window_Main.Height;
#endif
}

/*########################################################################################################################*
*-----------------------------------------------------Input handling------------------------------------------------------*
*#########################################################################################################################*/
static void VirtualKeyboard_Clamp(void) {
	int perRow = kb->cellsPerRow, numRows = kb->numRows;
	if (kb_curX < 0)       kb_curX += perRow;
	if (kb_curX >= perRow) kb_curX -= perRow;

	if (kb_curY < 0)        kb_curY += numRows;
	if (kb_curY >= numRows) kb_curY -= numRows;
}

static void VirtualKeyboard_Scroll(int xDelta, int yDelta) {
	if (kb_curX < 0) kb_curX = 0;

	kb_curX += xDelta;
	kb_curY += yDelta;
	
	VirtualKeyboard_Clamp();
	KB_MarkDirty();
}

static void VirtualKeyboard_ToggleTable(void) {
	kb->table = kb->table == kb->lower ? kb->upper : kb->lower;
	KB_MarkDirty();
}

static void VirtualKeyboard_AppendChar(char c) {
	String_Append(&kb_str, c);

	Event_RaiseString(&InputEvents.TextChanged, &kb_str);
	if (kb_shift) { VirtualKeyboard_ToggleTable(); kb_shift = false; }
}

static void VirtualKeyboard_Backspace(void) {
	if (kb_str.length) kb_str.length--;
	Event_RaiseString(&InputEvents.TextChanged, &kb_str);
	KB_MarkDirty();
}

static void VirtualKeyboard_ClickSelected(void) {
	int selected = VirtualKeyboard_GetSelected();
	if (selected < 0) return;

	switch (KB_GetBehaviour(selected)) {
	case KB_B_BACK:
		VirtualKeyboard_Backspace();
		break;
	case KB_B_ENTER:
		Input_SetPressed(CCKEY_ENTER);
		Input_SetReleased(CCKEY_ENTER);
		OnscreenKeyboard_Close();
		break;

	case KB_B_CAPS:
		VirtualKeyboard_ToggleTable();
		break;
	case KB_B_SHIFT:
		VirtualKeyboard_ToggleTable();
		kb_shift = true;
		break;
	case KB_B_SPACE:
		VirtualKeyboard_AppendChar(' ');
		break;
	case KB_B_CLOSE:
		OnscreenKeyboard_Close();
		break;

	default:
		VirtualKeyboard_AppendChar(kb->table[selected][0]);
		break;
	}
}

static cc_bool VirtualKeyboard_OnInputDown(int key, struct InputDevice* device) {
	int deltaX, deltaY;
	Input_CalcDelta(key, device, &deltaX, &deltaY);

	if (deltaX || deltaY) {
		VirtualKeyboard_Scroll(deltaX, deltaY);
	} else if (key == CCPAD_START  || key == CCPAD_1) {
		VirtualKeyboard_ClickSelected();
	} else if (key == CCPAD_SELECT || key == CCPAD_2) {
		VirtualKeyboard_Close();
	} else if (key == CCPAD_3) {
		VirtualKeyboard_Backspace();
	} else if (key == CCPAD_4) {
		VirtualKeyboard_AppendChar('@');
	} else if (key == CCPAD_L) {
		VirtualKeyboard_AppendChar(' ');
	} else if (key == CCPAD_R) {
		VirtualKeyboard_AppendChar('/');
	}
	return true;
}

static void VirtualKeyboard_PadAxis(void* obj, int port, int axis, float x, float y) {
	int xSteps, ySteps;

	xSteps = Utils_AccumulateWheelDelta(&kb_padXAcc, x / 100.0f);
	if (xSteps) VirtualKeyboard_Scroll(xSteps > 0 ? 1 : -1, 0);

	ySteps = Utils_AccumulateWheelDelta(&kb_padYAcc, y / 100.0f);
	if (ySteps) VirtualKeyboard_Scroll(0, ySteps > 0 ? 1 : -1);
}

static cc_bool VirtualKeyboard_GetPointerPosition(int idx, int* kbX, int* kbY) {
	int width  = VirtualKeyboard_Width();
	int height = VirtualKeyboard_Height();
	int originX, originY;
	VirtualKeyboard_CalcPosition(&originX, &originY, VirtualKeyboard_WindowWidth(), VirtualKeyboard_WindowHeight());

	int x = Pointers[idx].x, y = Pointers[idx].y;
	if (x < originX || y < originY || x >= originX + width || y >= originY + height) return false;

	*kbX = x - originX;
	*kbY = y - originY;
	return true;
}

static cc_bool VirtualKeyboard_PointerMove(int idx) {
	int kbX, kbY;
	if (!VirtualKeyboard_GetPointerPosition(idx, &kbX, &kbY)) return false;

	if (kb_clicking) return true;
	kb_clicking = true;

	kb_curX = kbX / kb_tileWidth;
	kb_curY = kbY / kb_tileHeight;
	if (kb_curX >= kb->cellsPerRow) kb_curX = kb->cellsPerRow - 1;

	VirtualKeyboard_Clamp();
	VirtualKeyboard_ClickSelected();
	return true;
}

static cc_bool VirtualKeyboard_PointerUp(int idx) {
	int kbX, kbY;
	kb_clicking = false;
	kb_curX     = -1;
	return VirtualKeyboard_GetPointerPosition(idx, &kbX, &kbY);
}


/*########################################################################################################################*
*--------------------------------------------------------2D mode----------------------------------------------------------*
*#########################################################################################################################*/
extern Rect2D dirty_rect;

static void VirtualKeyboard_MarkDirty2D(void) {
	LBackend_MarkAllDirty();
}

static void VirtualKeyboard_Display2D(struct Context2D* real_ctx) {
	struct Context2D ctx;
	struct Bitmap copy = real_ctx->bmp;
	int x, y;

	if (!DisplayInfo.ShowingSoftKeyboard) return;
	LBackend_MarkAllDirty();

	VirtualKeyboard_CalcPosition(&x, &y, copy.width, copy.height);
	copy.scan0 = Bitmap_GetRow(&real_ctx->bmp, y);
	copy.scan0 += x;

	Context2D_Wrap(&ctx, &copy);
	VirtualKeyboard_Draw(&ctx);
}

static void VirtualKeyboard_Close2D(void) {
	LBackend_Hooks[0]   = NULL;
	LBackend_Redraw();
}

/*########################################################################################################################*
*--------------------------------------------------------3D mode----------------------------------------------------------*
*#########################################################################################################################*/
static struct Texture kb_texture;
static GfxResourceID kb_vb;

static void VirtualKeyboard_MarkDirty3D(void) {
	Gfx_DeleteTexture(&kb_texture.ID);
}

static void VirtualKeyboard_Close3D(void) {
	Gfx_DeleteTexture(&kb_texture.ID);
	Game.Draw2DHooks[0] = NULL;
}

static void VirtualKeyboard_MakeTexture(void) {
	struct Context2D ctx;
	int width  = VirtualKeyboard_Width();
	int height = VirtualKeyboard_Height();
	Context2D_Alloc(&ctx, width, height);
	{
		VirtualKeyboard_Draw(&ctx);
		Context2D_MakeTexture(&kb_texture, &ctx);
	}
	Context2D_Free(&ctx);
	
	int x, y;
	VirtualKeyboard_CalcPosition(&x, &y, VirtualKeyboard_WindowWidth(), VirtualKeyboard_WindowHeight());
	kb_texture.x = x; kb_texture.y = y;
}

/* TODO hook into context lost etc */
static void VirtualKeyboard_Display3D(float delta) {
	if (!DisplayInfo.ShowingSoftKeyboard) return;
	
	if (!kb_vb) {
		kb_vb = Gfx_CreateDynamicVb(VERTEX_FORMAT_TEXTURED, 4);
		if (!kb_vb) return;
	}	
	
	if (!kb_texture.ID) {
		VirtualKeyboard_MakeTexture();
		if (!kb_texture.ID) return;
	}
	
	Gfx_3DS_SetRenderScreen(BOTTOM_SCREEN);
	Gfx_SetVertexFormat(VERTEX_FORMAT_TEXTURED);
	Gfx_BindTexture(kb_texture.ID);
	
	struct VertexTextured* data = (struct VertexTextured*)Gfx_LockDynamicVb(kb_vb, VERTEX_FORMAT_TEXTURED, 4);
	struct VertexTextured** ptr = &data;
	Gfx_Make2DQuad(&kb_texture, PACKEDCOL_WHITE, ptr);
	Gfx_UnlockDynamicVb(kb_vb);
	Gfx_DrawVb_IndexedTris(4);
	Gfx_3DS_SetRenderScreen(TOP_SCREEN);
}

/*########################################################################################################################*
*--------------------------------------------------------General----------------------------------------------------------*
*#########################################################################################################################*/
static void VirtualKeyboard_Hook(void) {
	/* Don't hook immediately into events, otherwise the initial up/down press that opened */
	/*  the virtual keyboard in the first place gets mistakenly processed */
	kb_needsHook = false;
	Event_Register_(&ControllerEvents.AxisUpdate, NULL, VirtualKeyboard_PadAxis);
	PointerHooks.MoveHook = VirtualKeyboard_PointerMove;
	PointerHooks.UpHook   = VirtualKeyboard_PointerUp;
}

static void VirtualKeyboard_Open(struct OpenKeyboardArgs* args, cc_bool launcher) {
	VirtualKeyboard_Close();
	VirtualKeyboard_Init();
	DisplayInfo.ShowingSoftKeyboard = true;

	kb_needsHook = true;
	kb_curX      = -1;
	kb_curY      = 0;
	kb_padXAcc   = 0;
	kb_padYAcc   = 0;
	kb_shift     = false;
	kb_yOffset   = args->yOffset;

	int mode = args->type & 0xFF;
	int num  = mode == KEYBOARD_TYPE_INTEGER || mode == KEYBOARD_TYPE_NUMBER;
	kb = num ? &numpad_layout : &normal_layout;

	kb_str.length = 0;
	String_AppendString(&kb_str, args->text);

	if (launcher) {
		KB_MarkDirty = VirtualKeyboard_MarkDirty2D;
	} else {
		KB_MarkDirty = VirtualKeyboard_MarkDirty3D;
	}

	Window_Main.SoftKeyboardFocus = true;
	Input.DownHook = VirtualKeyboard_OnInputDown;
	LBackend_Hooks[0]   = VirtualKeyboard_Display2D;
	Game.Draw2DHooks[0] = VirtualKeyboard_Display3D;
}

static void VirtualKeyboard_SetText(const cc_string* text) {
	if (!DisplayInfo.ShowingSoftKeyboard) return;
	String_Copy(&kb_str, text);
}

static void VirtualKeyboard_Close(void) {
	/* TODO find a better way */
	if (KB_MarkDirty == VirtualKeyboard_MarkDirty2D)
		VirtualKeyboard_Close2D();
	if (KB_MarkDirty == VirtualKeyboard_MarkDirty3D)
		VirtualKeyboard_Close3D();
		
	Event_Unregister_(&ControllerEvents.AxisUpdate, NULL, VirtualKeyboard_PadAxis);
	PointerHooks.MoveHook = NULL;
	PointerHooks.UpHook   = NULL;
	Window_Main.SoftKeyboardFocus = false;

	KB_MarkDirty   = NULL;
	kb_needsHook   = false;
	Input.DownHook = NULL;

	DisplayInfo.ShowingSoftKeyboard = false;
}
