#include "Screens.h"
#include "Widgets.h"
#include "Game.h"
#include "Event.h"
#include "GraphicsCommon.h"
#include "Platform.h"
#include "Inventory.h"
#include "Drawer2D.h"

void Screen_FreeWidgets(Widget** widgets, UInt32 widgetsCount) {
	if (widgets == NULL) return;
	UInt32 i;
	for (i = 0; i < widgetsCount; i++) {
		if (widgets[i] == NULL) continue;
		widgets[i]->Base.Free(&widgets[i]->Base);
	}
}

void Screen_RepositionWidgets(Widget** widgets, UInt32 widgetsCount) {
	if (widgets == NULL) return;
	UInt32 i;
	for (i = 0; i < widgetsCount; i++) {
		if (widgets[i] == NULL) continue;
		widgets[i]->Reposition(widgets[i]);
	}
}

void Screen_RenderWidgets(Widget** widgets, UInt32 widgetsCount, Real64 delta) {
	if (widgets == NULL) return;

	UInt32 i;
	for (i = 0; i < widgetsCount; i++) {
		if (widgets[i] == NULL) continue;
		widgets[i]->Base.Render(&widgets[i]->Base, delta);
	}
}



/* These were sourced by taking a screenshot of vanilla
   Then using paint to extract the colour components
   Then using wolfram alpha to solve the glblendfunc equation */
PackedCol Menu_TopBackCol    = PACKEDCOL_CONST(24, 24, 24, 105);
PackedCol Menu_BottomBackCol = PACKEDCOL_CONST(51, 51, 98, 162);

void ClickableScreen_RenderMenuBounds(void) {
	GfxCommon_Draw2DGradient(0, 0, Game_Width, Game_Height, Menu_TopBackCol, Menu_BottomBackCol);
}

void ClickableScreen_DefaultWidgetSelected(GuiElement* elem, Widget* widget) { }

bool ClickableScreen_HandlesMouseDown(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	ClickableScreen* screen = (ClickableScreen*)elem;
	UInt32 i;
	/* iterate backwards (because last elements rendered are shown over others) */
	for (i = screen->WidgetsCount; i > 0; i--) {
		Widget* widget = screen->Widgets[i - 1];
		if (widget != NULL && Gui_Contains(widget->X, widget->Y, widget->Width, widget->Height, x, y)) {
			if (!widget->Disabled) {
				elem = &widget->Base;
				elem->HandlesMouseDown(elem, x, y, btn);
			}
			return true;
		}
	}
	return false;
}

bool ClickableScreens_HandleMouseMove(GuiElement* elem, Int32 x, Int32 y) {
	ClickableScreen* screen = (ClickableScreen*)elem;
	if (screen->LastX == x && screen->LastY == y) return true;
	UInt32 i;
	for (i = 0; i < screen->WidgetsCount; i++) {
		Widget* widget = screen->Widgets[i];
		if (widget != NULL) widget->Active = false;
	}

	for (i = screen->WidgetsCount; i > 0; i--) {
		Widget* widget = screen->Widgets[i];
		if (widget != NULL && Gui_Contains(widget->X, widget->Y, widget->Width, widget->Height, x, y)) {
			widget->Active = true;
			screen->LastX = x; screen->LastY = y;
			screen->OnWidgetSelected(elem, widget);
			return true;
		}
	}

	screen->LastX = x; screen->LastY = y;
	screen->OnWidgetSelected(elem, NULL);
	return false;
}

void ClickableScreen_Create(ClickableScreen* screen) {
	Screen_Reset(&screen->Base);
	screen->Base.Base.HandlesMouseDown = ClickableScreen_HandlesMouseDown;
	screen->Base.Base.HandlesMouseMove = ClickableScreens_HandleMouseMove;

	screen->LastX = -1; screen->LastY = -1;
	screen->OnWidgetSelected = ClickableScreen_DefaultWidgetSelected;
}


typedef struct InventoryScreen_ {
	Screen Base;
	FontDesc Font;
	TableWidget Table;
	bool ReleasedInv;
} InventoryScreen;
InventoryScreen InventoryScreen_Instance;

void InventoryScreen_OnBlockChanged(void) {
	TableWidget_OnInventoryChanged(&InventoryScreen_Instance.Table);
}

void InventoryScreen_ContextLost(void) {
	GuiElement* elem = &InventoryScreen_Instance.Table.Base.Base;
	elem->Free(elem);
}

void InventoryScreen_ContextRecreated(void) {
	GuiElement* elem = &InventoryScreen_Instance.Table.Base.Base;
	elem->Recreate(elem);
}

void InventoryScreen_Init(GuiElement* elem) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	FontDesc_Make(&screen->Font, 16, FONT_STYLE_NORMAL);
	Platform_MakeFont(&screen->Font, &Game_FontName);

	elem = &screen->Table.Base.Base;
	TableWidget_Create(&screen->Table);
	screen->Table.Font = screen->Font;
	screen->Table.ElementsPerRow = Game_PureClassic ? 9 : 10;
	elem->Init(elem);

	Key_KeyRepeat = true;
	Event_RegisterVoid(&BlockEvents_PermissionsChanged, InventoryScreen_OnBlockChanged);
	Event_RegisterVoid(&BlockEvents_BlockDefChanged, InventoryScreen_OnBlockChanged);	
	Event_RegisterVoid(&GfxEvents_ContextLost, InventoryScreen_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, InventoryScreen_ContextRecreated);
}

void InventoryScreen_Render(GuiElement* elem, Real64 delta) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	elem = &screen->Table.Base.Base;
	elem->Render(elem, delta);
}

void InventoryScreen_OnResize(Screen* elem) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	Widget* widget = &screen->Table.Base;
	widget->Reposition(widget);
}

void InventoryScreen_Free(GuiElement* elem) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	Platform_FreeFont(&screen->Font);
	elem = &screen->Table.Base.Base;
	elem->Free(elem);

	Key_KeyRepeat = false;
	Event_UnregisterVoid(&BlockEvents_PermissionsChanged, InventoryScreen_OnBlockChanged);
	Event_UnregisterVoid(&BlockEvents_BlockDefChanged,    InventoryScreen_OnBlockChanged);
	Event_UnregisterVoid(&GfxEvents_ContextLost,          InventoryScreen_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated,     InventoryScreen_ContextRecreated);
}

bool InventoryScreen_HandlesKeyDown(GuiElement* elem, Key key) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	TableWidget* table = &screen->Table;
	elem = &screen->Table.Base.Base;

	if (key == KeyBind_Get(KeyBind_PauseOrExit)) {
		Gui_SetNewScreen(NULL);
	} else if (key == KeyBind_Get(KeyBind_Inventory) && screen->ReleasedInv) {
		Gui_SetNewScreen(NULL);
	} else if (key == Key_Enter && table->SelectedIndex != -1) {
		Inventory_SetSelectedBlock(table->Elements[table->SelectedIndex]);
		Gui_SetNewScreen(NULL);
	} else if (elem->HandlesKeyDown(elem, key)) {
	} else {
		game.Gui.hudScreen.hotbar.HandlesKeyDown(key);
	}
	return true;
}

bool InventoryScreen_HandlesKeyUp(GuiElement* elem, Key key) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	if (key == KeyBind_Get(KeyBind_Inventory)) {
		screen->ReleasedInv = true; return true;
	}
	return game.Gui.hudScreen.hotbar.HandlesKeyUp(key);
}

bool InventoryScreen_HandlesMouseDown(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	TableWidget* table = &screen->Table;
	elem = &screen->Table.Base.Base;
	if (table->Scroll.DraggingMouse || game.Gui.hudScreen.hotbar.HandlesMouseDown(x, y, btn))
		return true;

	bool handled = elem->HandlesMouseDown(elem, x, y, btn);
	if ((!handled || table->PendingClose) && btn == MouseButton_Left) {
		bool hotbar = Key_IsControlPressed();
		if (!hotbar) Gui_SetNewScreen(NULL);
	}
	return true;
}

bool InventoryScreen_HandlesMouseUp(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	elem = &screen->Table.Base.Base;
	return elem->HandlesMouseUp(elem, x, y, btn);
}

bool InventoryScreen_HandlesMouseMove(GuiElement* elem, Int32 x, Int32 y) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	elem = &screen->Table.Base.Base;
	return elem->HandlesMouseMove(elem, x, y);
}

bool InventoryScreen_HandlesMouseScroll(GuiElement* elem, Real32 delta) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	elem = &screen->Table.Base.Base;

	bool hotbar = Key_IsAltPressed() || Key_IsControlPressed() || Key_IsShiftPressed();
	if (hotbar) return false;
	return elem->HandlesMouseScroll(elem, delta);
}

Screen* InventoryScreen_MakeInstance(void) {
	InventoryScreen* screen = &InventoryScreen_Instance;
	Platform_MemSet(&screen, 0, sizeof(InventoryScreen));
	Screen_Reset(&screen->Base);

	screen->Base.Base.HandlesKeyDown     = InventoryScreen_HandlesKeyDown;
	screen->Base.Base.HandlesKeyUp       = InventoryScreen_HandlesKeyUp;
	screen->Base.Base.HandlesMouseDown   = InventoryScreen_HandlesMouseDown;
	screen->Base.Base.HandlesMouseUp     = InventoryScreen_HandlesMouseUp;
	screen->Base.Base.HandlesMouseMove   = InventoryScreen_HandlesMouseMove;
	screen->Base.Base.HandlesMouseScroll = InventoryScreen_HandlesMouseScroll;

	screen->Base.OnResize           = InventoryScreen_OnResize;
	screen->Base.Base.Init          = InventoryScreen_Init;
	screen->Base.Base.Render        = InventoryScreen_Render;
	screen->Base.Base.Free          = InventoryScreen_Free;
	return &screen->Base;
}
extern Screen* InventoryScreen_UNSAFE_RawPointer = &InventoryScreen_Instance.Base;


typedef struct StatusScreen_ {
	Screen Base;
	FontDesc Font;
	UInt8 StatusText[String_BufferSize(STRING_SIZE * 2)];
} StatusScreen;

namespace ClassicalSharp.Gui.Screens{
	public class StatusScreen : Screen, IGameComponent {

	Font font;
	StringBuffer statusBuffer;

	public StatusScreen(Game game) : base(game) {
		statusBuffer = new StringBuffer(128);
	}

	public void Init(Game game) { }
	public void Ready(Game game) { Init(); }
	public void Reset(Game game) { }
	public void OnNewMap(Game game) { }
	public void OnNewMapLoaded(Game game) { }

	TextWidget status, hackStates;
	TextAtlas posAtlas;
	public override void Render(double delta) {
		UpdateStatus(delta);
		if (game.HideGui || !game.ShowFPS) return;

		gfx.Texturing = true;
		status.Render(delta);
		if (!game.ClassicMode && game.Gui.activeScreen == null) {
			UpdateHackState(false);
			DrawPosition();
			hackStates.Render(delta);
		}
		gfx.Texturing = false;
	}

	double accumulator;
	int frames, totalSeconds;

	void UpdateStatus(double delta) {
		frames++;
		accumulator += delta;
		if (accumulator < 1) return;

		int index = 0;
		totalSeconds++;
		int fps = (int)(frames / accumulator);

		statusBuffer.Clear()
			.AppendNum(ref index, fps).Append(ref index, " fps, ");
		if (game.ClassicMode) {
			statusBuffer.AppendNum(ref index, game.ChunkUpdates).Append(ref index, " chunk updates");
		} else {
			if (game.ChunkUpdates > 0) {
				statusBuffer.AppendNum(ref index, game.ChunkUpdates).Append(ref index, " chunks/s, ");
			}
			int indices = (game.Vertices >> 2) * 6;
			statusBuffer.AppendNum(ref index, indices).Append(ref index, " vertices");

			int ping = PingList.AveragePingMilliseconds();
			if (ping != 0) {
				statusBuffer.Append(ref index, ", ping ").AppendNum(ref index, ping).Append(ref index, " ms");
			}
		}

		status.SetText(statusBuffer.ToString());
		accumulator = 0;
		frames = 0;
		game.ChunkUpdates = 0;
	}

	public override void Init() {
		font = new Font(game.FontName, 16);
		ContextRecreated();

		game.Events.ChatFontChanged += ChatFontChanged;
		gfx.ContextLost += ContextLost;
		gfx.ContextRecreated += ContextRecreated;
	}

	protected override void ContextLost() {
		status.Dispose();
		posAtlas.Dispose();
		hackStates.Dispose();
	}

	protected override void ContextRecreated() {
		status = new TextWidget(game, font)
			.SetLocation(Anchor.LeftOrTop, Anchor.LeftOrTop, 2, 2);
		status.ReducePadding = true;
		status.Init();
		string msg = statusBuffer.Length > 0 ? statusBuffer.ToString() : "FPS: no data yet";
		status.SetText(msg);

		posAtlas = new TextAtlas(game, 16);
		posAtlas.Pack("0123456789-, ()", font, "Position: ");
		posAtlas.tex.Y = (short)(status.Height + 2);

		int yOffset = status.Height + posAtlas.tex.Height + 2;
		hackStates = new TextWidget(game, font)
			.SetLocation(Anchor.LeftOrTop, Anchor.LeftOrTop, 2, yOffset);
		hackStates.ReducePadding = true;
		hackStates.Init();
		UpdateHackState(true);
	}

	public override void Dispose() {
		font.Dispose();
		ContextLost();

		game.Events.ChatFontChanged -= ChatFontChanged;
		gfx.ContextLost -= ContextLost;
		gfx.ContextRecreated -= ContextRecreated;
	}

	void ChatFontChanged(object sender, EventArgs e) { Recreate(); }

	public override void OnResize(int width, int height) { }

	void DrawPosition() {
		int index = 0;
		Texture tex = posAtlas.tex;
		tex.X1 = 2; tex.Width = (ushort)posAtlas.offset;
		IGraphicsApi.Make2DQuad(ref tex, FastColour.WhitePacked,
			game.ModelCache.vertices, ref index);

		Vector3I pos = Vector3I.Floor(game.LocalPlayer.Position);
		posAtlas.curX = posAtlas.offset + 2;
		VertexP3fT2fC4b[] vertices = game.ModelCache.vertices;

		posAtlas.Add(13, vertices, ref index);
		posAtlas.AddInt(pos.X, vertices, ref index);
		posAtlas.Add(11, vertices, ref index);
		posAtlas.AddInt(pos.Y, vertices, ref index);
		posAtlas.Add(11, vertices, ref index);
		posAtlas.AddInt(pos.Z, vertices, ref index);
		posAtlas.Add(14, vertices, ref index);

		gfx.BindTexture(posAtlas.tex.ID);
		gfx.UpdateDynamicVb_IndexedTris(game.ModelCache.vb, game.ModelCache.vertices, index);
	}

	bool speeding, halfSpeeding, noclip, fly;
	int lastFov;
	void UpdateHackState(bool force) {
		HacksComponent hacks = game.LocalPlayer.Hacks;
		if (force || hacks.Speeding != speeding || hacks.HalfSpeeding != halfSpeeding || hacks.Noclip != noclip ||
			hacks.Flying != fly || game.Fov != lastFov) {
			speeding = hacks.Speeding; halfSpeeding = hacks.HalfSpeeding; noclip = hacks.Noclip; fly = hacks.Flying;
			lastFov = game.Fov;
			int index = 0;
			statusBuffer.Clear();

			if (game.Fov != game.DefaultFov) statusBuffer.Append(ref index, "Zoom fov ")
				.AppendNum(ref index, lastFov).Append(ref index, "  ");
			if (fly) statusBuffer.Append(ref index, "Fly ON   ");

			bool speed = (speeding || halfSpeeding) &&
				(hacks.CanSpeed || hacks.BaseHorSpeed > 1);
			if (speed) statusBuffer.Append(ref index, "Speed ON   ");
			if (noclip) statusBuffer.Append(ref index, "Noclip ON   ");
			hackStates.SetText(statusBuffer.ToString());
		}
	}
}
}
