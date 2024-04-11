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

static cc_bool kb_inited, kb_showing, kb_shift, kb_needsHook;
static struct FontDesc kb_font;
static int kb_selected;
static const char** kb_table;
static float kb_padXAcc, kb_padYAcc;
static char kb_buffer[512];
static cc_string kb_str = String_FromArray(kb_buffer);
static void (*KB_MarkDirty)(void);

#define KB_CELLS_PER_ROW 13
#define KB_LAST_CELL     (KB_CELLS_PER_ROW - 1)
#define KB_TOTAL_ROWS     5
#define KB_LAST_ROW      (KB_TOTAL_ROWS - 1)

#define KB_TOTAL_CHARS (KB_CELLS_PER_ROW * 4) + 3
#define KB_INDEX(x, y) ((y) * KB_CELLS_PER_ROW + (x))

#define KB_TILE_SIZE     32

static const char* kb_table_lower[] =
{
	"1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "Backspace",
	"q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "(", ")", "&    ",
	"a", "s", "d", "f", "g", "h", "j", "k", "l", "?", ";", "'", "Enter",
	"z", "x", "c", "v", "b", "n", "m", ".", ",","\\", "!", "@", "/    ",
	"Caps", "Shift", "Space"
};
static const char* kb_table_upper[] =
{
	"1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "Backspace",
	"Q", "W", "E", "R", "T", "Y", "U", "I", "o", "p", "(", ")", "&   ",
	"A", "S", "D", "F", "G", "H", "J", "K", "L", "?", ";", "'", "Enter",
	"Z", "X", "C", "V", "B", "N", "M", ".", ",","\\", "!", "@", "/    ",
	"Caps", "Shift", "Space"
};

extern void LWidget_DrawBorder(struct Context2D* ctx, BitmapCol color, int insetX, int insetY,
								int x, int y, int width, int height);

static void VirtualKeyboard_Init(void) {
	if (kb_inited) return;
	kb_inited = true;
	Font_Make(&kb_font, 16, FONT_FLAGS_NONE);
}

static int VirtualKeyboard_Width(void) {
	return (KB_CELLS_PER_ROW + 3) * KB_TILE_SIZE;
}

static int VirtualKeyboard_Height(void) {
	return KB_TOTAL_ROWS * KB_TILE_SIZE;
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
	int i, x, y, w, h, dx, dy;
	Drawer2D.Colors['f'] = Drawer2D.Colors['0'];
	if (kb_needsHook) VirtualKeyboard_Hook();

	for (row = 0, y = 0; row < KB_TOTAL_ROWS; row++)
	{
		for (cell = 0, x = 0; cell < KB_CELLS_PER_ROW; cell++)
		{
			i = row * KB_CELLS_PER_ROW + cell;
			if (i >= KB_TOTAL_CHARS) break;

			str = String_FromReadonly(kb_table[i]);
			DrawTextArgs_Make(&args, &str, &kb_font, false);
			w = KB_TILE_SIZE * (str.length > 1 ? 4 : 1);
			h = KB_TILE_SIZE;

			Gradient_Noise(ctx, i == kb_selected ? KB_SELECTED_COLOR : KB_NORMAL_COLOR, 4, x, y, w, h);
			LWidget_DrawBorder(ctx, KB_BACKGROUND_COLOR, 1, 1, x, y, w, h);

			dx = (w - Drawer2D_TextWidth (&args)) / 2;
			dy = (h - Drawer2D_TextHeight(&args)) / 2;
			Context2D_DrawText(ctx, &args, x + dx, y + dy);

			x += w;
		}
		y += KB_TILE_SIZE;
	}
	
	Drawer2D.Colors['f'] = Drawer2D.Colors['F'];
}

static void VirtualKeyboard_Scroll(int delta) {
	if (kb_selected < 0) kb_selected = 0;

	kb_selected += delta;
	if (kb_selected < 0) kb_selected += KB_TOTAL_CHARS;
	Math_Clamp(kb_selected, 0, KB_TOTAL_CHARS - 1);
	KB_MarkDirty();
}

static void VirtualKeyboard_ToggleTable(void) {
	kb_table = kb_table == kb_table_lower ? kb_table_upper : kb_table_lower;
	KB_MarkDirty();
}

static void VirtualKeyboard_AppendChar(char c) {
	String_Append(&kb_str, c);

	Event_RaiseString(&InputEvents.TextChanged, &kb_str);
	if (kb_shift) { VirtualKeyboard_ToggleTable(); kb_shift = false; }
}

static void VirtualKeyboard_ClickSelected(void) {
	if (kb_selected < 0) return;

	/* TODO kinda hacky, redo this */
	switch (kb_selected) {
	case KB_INDEX(KB_LAST_CELL, 0):
		if (kb_str.length) kb_str.length--;
		Event_RaiseString(&InputEvents.TextChanged, &kb_str);
		KB_MarkDirty();
		break;
	case KB_INDEX(KB_LAST_CELL, 2):
		KB_MarkDirty();
		OnscreenKeyboard_Close();
		Input_SetPressed(CCKEY_ENTER);
		Input_SetReleased(CCKEY_ENTER);
		break;

	case KB_INDEX(0, KB_LAST_ROW):
		VirtualKeyboard_ToggleTable();
		break;
	case KB_INDEX(1, KB_LAST_ROW):
		VirtualKeyboard_ToggleTable();
		kb_shift = true;
		break;
	case KB_INDEX(2, KB_LAST_ROW):
		VirtualKeyboard_AppendChar(' ');
		break;

	default:
		VirtualKeyboard_AppendChar(kb_table[kb_selected][0]);
		break;
	}
}

static void VirtualKeyboard_ProcessDown(void* obj, int key, cc_bool was) {
	if (Input_IsLeftButton(key)) {
		VirtualKeyboard_Scroll(-1);
	} else if (Input_IsRightButton(key)) {
		VirtualKeyboard_Scroll(+1);
	} else if (Input_IsUpButton(key)) {
		VirtualKeyboard_Scroll(-KB_CELLS_PER_ROW);
	} else if (Input_IsDownButton(key)) {
		VirtualKeyboard_Scroll(+KB_CELLS_PER_ROW);
	} else if (Input_IsEnterButton(key)  || key == CCPAD_A) {
		VirtualKeyboard_ClickSelected();
	} else if (Input_IsEscapeButton(key) || key == CCPAD_B) {
		VirtualKeyboard_Close();
	}
}

static void VirtualKeyboard_PadAxis(void* obj, int axis, float x, float y) {
	int xSteps, ySteps;

	xSteps = Utils_AccumulateWheelDelta(&kb_padXAcc, x / 100.0f);
	if (xSteps) VirtualKeyboard_Scroll(xSteps > 0 ? 1 : -1);

	ySteps = Utils_AccumulateWheelDelta(&kb_padYAcc, y / 100.0f);
	if (ySteps) VirtualKeyboard_Scroll(ySteps > 0 ? KB_CELLS_PER_ROW : -KB_CELLS_PER_ROW);
}


extern Rect2D dirty_rect;
static void VirtualKeyboard_MarkDirty2D(void) {
	if (!dirty_rect.width) dirty_rect.width = 2;
}

static void VirtualKeyboard_MarkDirty3D(void) {
	/* TODO */
}

static void VirtualKeyboard_Hook(void) {
	/* Don't hook immediately into events, otherwise the initial up/down press that opened */
	/*  the virtual keyboard in the first place gets mistakenly processed */
	kb_needsHook = false;

	Event_Register_(&InputEvents.Down,            NULL, VirtualKeyboard_ProcessDown);
	Event_Register_(&ControllerEvents.AxisUpdate, NULL, VirtualKeyboard_PadAxis);
}

static void VirtualKeyboard_Open(struct OpenKeyboardArgs* args, cc_bool launcher) {
	VirtualKeyboard_Close();
	VirtualKeyboard_Init();
	kb_showing   = true;
	kb_needsHook = true;

	kb_table    = kb_table_lower;
	kb_selected = -1;
	kb_padXAcc  = 0;
	kb_padYAcc  = 0;
	kb_shift    = false;

	kb_str.length = 0;
	if (args) String_AppendConst(&kb_str, args->placeholder);

	if (launcher) {
		KB_MarkDirty = VirtualKeyboard_MarkDirty2D;
	} else {
		KB_MarkDirty = VirtualKeyboard_MarkDirty3D;
	}

	Window_Main.SoftKeyboardFocus = true;
}

static void VirtualKeyboard_SetText(const cc_string* text) {
	if (!kb_showing) return;
	String_Copy(&kb_str, text);
}

static void VirtualKeyboard_Close(void) {
	if (KB_MarkDirty) KB_MarkDirty();
	Event_Unregister_(&InputEvents.Down,            NULL, VirtualKeyboard_ProcessDown);
	Event_Unregister_(&ControllerEvents.AxisUpdate, NULL, VirtualKeyboard_PadAxis);
	Window_Main.SoftKeyboardFocus = false;

	KB_MarkDirty = NULL;
	kb_showing   = false;
	kb_needsHook = false;
}

static void VirtualKeyboard_Display2D(Rect2D* r, struct Bitmap* bmp) {
	struct Context2D ctx;
	struct Bitmap copy = *bmp;
	int x, y;

	/* Mark entire framebuffer as needing to be redrawn */
	r->x = 0; r->width  = bmp->width;
	r->y = 0; r->height = bmp->height;

	/* Draw virtual keyboard at centre of window bottom */
	y = bmp->height - 1 - VirtualKeyboard_Height();
	y = max(y, 0);
	copy.scan0 = Bitmap_GetRow(bmp, y);

	x = (bmp->width - VirtualKeyboard_Width()) / 2;
	x = max(x, 0);
	copy.scan0 += x;

	Context2D_Wrap(&ctx, &copy);
	VirtualKeyboard_Draw(&ctx);
}