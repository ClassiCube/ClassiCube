#include "SurvivalInv.h"
#include "Survival.h"
#include "Game.h"
#include "Server.h"
#include "Inventory.h"
#include "Input.h"
#include "Event.h"
#include "Gui.h"
#include "Screens.h"
#include "Drawer2D.h"
#include "Graphics.h"
#include "IsometricDrawer.h"
#include "BlockID.h"
#include "Block.h"
#include "Window.h"
#include "String_.h"
#include "Chat.h"
#include "Funcs.h"

/*########################################################################################################################*
*--------------------------------------------------Inventory data model--------------------------------------------------*
*#########################################################################################################################*/
struct SurvivalSlot SurvivalInv_Slots[SURINV_TOTAL_SLOTS];

cc_bool SurvivalInv_Add(BlockID block, int count) {
	int i;
	if (block == BLOCK_AIR) return false;

	/* Try to stack onto an existing partial slot */
	for (i = 0; i < SURINV_TOTAL_SLOTS; i++) {
		if (SurvivalInv_Slots[i].id != block) continue;
		if (SurvivalInv_Slots[i].count >= SURINV_MAX_STACK) continue;
		SurvivalInv_Slots[i].count += count;
		if (SurvivalInv_Slots[i].count > SURINV_MAX_STACK)
			SurvivalInv_Slots[i].count = SURINV_MAX_STACK;
		SurvivalInv_SyncHotbar();
		return true;
	}

	/* Find first empty slot */
	for (i = 0; i < SURINV_TOTAL_SLOTS; i++) {
		if (SurvivalInv_Slots[i].id != BLOCK_AIR && SurvivalInv_Slots[i].count > 0) continue;
		SurvivalInv_Slots[i].id    = block;
		SurvivalInv_Slots[i].count = count;
		SurvivalInv_SyncHotbar();
		return true;
	}

	return false;
}

void SurvivalInv_Remove(int slotIdx, int count) {
	if (slotIdx < 0 || slotIdx >= SURINV_TOTAL_SLOTS) return;
	SurvivalInv_Slots[slotIdx].count -= count;
	if (SurvivalInv_Slots[slotIdx].count <= 0) {
		SurvivalInv_Slots[slotIdx].id    = BLOCK_AIR;
		SurvivalInv_Slots[slotIdx].count = 0;
	}
	SurvivalInv_SyncHotbar();
}

void SurvivalInv_RemoveBlock(BlockID block, int count) {
	int i, take;
	for (i = 0; i < SURINV_TOTAL_SLOTS && count > 0; i++) {
		if (SurvivalInv_Slots[i].id != block) continue;
		take = count < SurvivalInv_Slots[i].count ? count : SurvivalInv_Slots[i].count;
		SurvivalInv_Remove(i, take);
		count -= take;
	}
}

/* The hotbar maps 1:1 onto inventory slots 0-8, so the selected hotbar */
/*  index is also the inventory slot index of the block in the player's hand */
void SurvivalInv_ConsumeSelected(void) {
	int idx = Inventory.SelectedIndex;
	if (idx < 0 || idx >= SURINV_HOTBAR_SLOTS) return;
	SurvivalInv_Remove(idx, 1);
}

int SurvivalInv_SelectedCount(void) {
	int idx = Inventory.SelectedIndex;
	if (idx < 0 || idx >= SURINV_HOTBAR_SLOTS) return 0;
	return SurvivalInv_Slots[idx].count;
}

int SurvivalInv_Count(BlockID block) {
	int i, total = 0;
	for (i = 0; i < SURINV_TOTAL_SLOTS; i++) {
		if (SurvivalInv_Slots[i].id == block)
			total += SurvivalInv_Slots[i].count;
	}
	return total;
}

void SurvivalInv_SyncHotbar(void) {
	int i;
	BlockID b;
	for (i = 0; i < SURINV_HOTBAR_SLOTS; i++) {
		b = (SurvivalInv_Slots[i].count > 0) ? SurvivalInv_Slots[i].id : BLOCK_AIR;
		Inventory_Set(i, b);
	}
}

void SurvivalInv_Clear(void) {
	int i;
	for (i = 0; i < SURINV_TOTAL_SLOTS; i++) {
		SurvivalInv_Slots[i].id    = BLOCK_AIR;
		SurvivalInv_Slots[i].count = 0;
	}
}

/*########################################################################################################################*
*--------------------------------------------------Inventory screen------------------------------------------------------*
*#########################################################################################################################*/
#define SI_SLOT   40
#define SI_GAP     4
#define SI_CELL   (SI_SLOT + SI_GAP)
#define SI_PAD     8
#define SI_TITLE_H 16
#define SI_MAIN_SEP 8
#define SI_COLS    9
#define SI_MAIN_ROWS 3
/* Total vertices for 36 block icons */
#define SI_MAX_VERTS  (SURINV_TOTAL_SLOTS * ISOMETRICDRAWER_MAXVERTICES)
#define SI_STATE_SIZE (SI_MAX_VERTS / 4)

static struct SvInvScreen {
	Screen_Body
	struct FontDesc font;
	struct Texture  titleTex;
	struct Texture  countTex[SURINV_TOTAL_SLOTS];
	int             countVal[SURINV_TOTAL_SLOTS];
	int             isoState[SI_STATE_SIZE];
	int             isoCount;
	int             panelX, panelY, panelW, panelH;
	int             heldSlot;
} SvInvScreen_Instance;

static cc_bool sv_inv_shown;

static const struct ScreenVTABLE SvInv_VTABLE;

static void SvInv_GetSlotPos(const struct SvInvScreen* s, int idx, int* x, int* y) {
	int mainTop = s->panelY + SI_PAD + SI_TITLE_H + SI_GAP;
	if (idx < SURINV_HOTBAR_SLOTS) {
		*x = s->panelX + SI_PAD + idx * SI_CELL;
		*y = mainTop + SI_MAIN_ROWS * SI_CELL + SI_MAIN_SEP;
	} else {
		int mi = idx - SURINV_HOTBAR_SLOTS;
		*x = s->panelX + SI_PAD + (mi % SI_COLS) * SI_CELL;
		*y = mainTop + (mi / SI_COLS) * SI_CELL;
	}
}

static void SvInv_RebuildCountTextures(struct SvInvScreen* s) {
	struct DrawTextArgs args;
	cc_string numStr;
	char numBuf[8];
	int i, count;

	for (i = 0; i < SURINV_TOTAL_SLOTS; i++) {
		count = (SurvivalInv_Slots[i].id != BLOCK_AIR) ? SurvivalInv_Slots[i].count : 0;
		if (count == s->countVal[i]) continue;

		Gfx_DeleteTexture(&s->countTex[i].ID);
		s->countVal[i] = count;
		if (count <= 1) continue;

		String_InitArray(numStr, numBuf);
		String_AppendInt(&numStr, count);
		DrawTextArgs_Make(&args, &numStr, &s->font, true);
		Drawer2D_MakeTextTexture(&s->countTex[i], &args);
	}
}

static void SvInv_BuildMesh(void* screen) {
	struct SvInvScreen* s = (struct SvInvScreen*)screen;
	struct VertexTextured* data;
	BlockID blk;
	int i, sx, sy;

	SvInv_RebuildCountTextures(s);

	data = Screen_LockVb(s);
	IsometricDrawer_BeginBatch(data, s->isoState);

	for (i = 0; i < SURINV_TOTAL_SLOTS; i++) {
		blk = SurvivalInv_Slots[i].id;
		if (blk == BLOCK_AIR || SurvivalInv_Slots[i].count <= 0) continue;
		SvInv_GetSlotPos(s, i, &sx, &sy);
		IsometricDrawer_AddBatch(blk, (float)(SI_SLOT / 2), sx + SI_SLOT / 2, sy + SI_SLOT / 2);
	}

	s->isoCount = IsometricDrawer_EndBatch();
	Gfx_UnlockDynamicVb(s->vb);
}

static void SvInv_Render(void* screen, float delta) {
	struct SvInvScreen* s = (struct SvInvScreen*)screen;
	PackedCol slotCol;
	int i, sx, sy;
	(void)delta;

	/* Panel background */
	Gfx_Draw2DFlat(s->panelX, s->panelY, s->panelW, s->panelH,
	               PackedCol_Make(0, 0, 0, 200));

	/* Slot backgrounds */
	for (i = 0; i < SURINV_TOTAL_SLOTS; i++) {
		SvInv_GetSlotPos(s, i, &sx, &sy);
		if (i == s->heldSlot)
			slotCol = PackedCol_Make(220, 220, 0, 220);
		else if (SurvivalInv_Slots[i].id != BLOCK_AIR && SurvivalInv_Slots[i].count > 0)
			slotCol = PackedCol_Make(80, 80, 80, 200);
		else
			slotCol = PackedCol_Make(40, 40, 40, 200);
		Gfx_Draw2DFlat(sx, sy, SI_SLOT, SI_SLOT, slotCol);
	}

	/* Title texture */
	if (s->titleTex.ID) Texture_Render(&s->titleTex);

	/* Block icons from VB */
	if (s->isoCount > 0) {
		Gfx_SetVertexFormat(VERTEX_FORMAT_TEXTURED);
		Gfx_BindDynamicVb(s->vb);
		IsometricDrawer_Render(s->isoCount, 0, s->isoState);
	}

	/* Stack count numbers */
	for (i = 0; i < SURINV_TOTAL_SLOTS; i++) {
		if (SurvivalInv_Slots[i].count <= 1) continue;
		if (!s->countTex[i].ID) continue;
		SvInv_GetSlotPos(s, i, &sx, &sy);
		s->countTex[i].x = (short)(sx + SI_SLOT - s->countTex[i].width);
		s->countTex[i].y = (short)(sy + SI_SLOT - s->countTex[i].height);
		Texture_Render(&s->countTex[i]);
	}
}

static void SvInv_Layout(void* screen) {
	struct SvInvScreen* s = (struct SvInvScreen*)screen;
	s->panelW = SI_PAD + SI_COLS * SI_CELL - SI_GAP + SI_PAD;
	s->panelH = SI_PAD + SI_TITLE_H + SI_GAP
	          + SI_MAIN_ROWS * SI_CELL
	          + SI_MAIN_SEP + SI_SLOT + SI_PAD;
	s->panelX = (Window_UI.Width  - s->panelW) / 2;
	s->panelY = (Window_UI.Height - s->panelH) / 2;

	s->titleTex.x = (short)(s->panelX + (s->panelW - s->titleTex.width) / 2);
	s->titleTex.y = (short)(s->panelY + SI_PAD);

	s->dirty = true;
}

static void SvInv_Init(void* screen) {
	struct SvInvScreen* s = (struct SvInvScreen*)screen;
	s->maxVertices = SI_MAX_VERTS;
	s->grabsInput  = true;
	s->closable    = true;
	s->widgets     = NULL;
	s->numWidgets  = 0;
	s->heldSlot    = -1;
}

static void SvInv_Free(void* screen) {
	sv_inv_shown = false;
}

static void SvInv_ContextLost(void* screen) {
	struct SvInvScreen* s = (struct SvInvScreen*)screen;
	int i;
	Font_Free(&s->font);
	Screen_ContextLost(s);
	Gfx_DeleteTexture(&s->titleTex.ID);
	for (i = 0; i < SURINV_TOTAL_SLOTS; i++) {
		Gfx_DeleteTexture(&s->countTex[i].ID);
		s->countVal[i] = -1;
	}
}

static void SvInv_ContextRecreated(void* screen) {
	struct SvInvScreen* s = (struct SvInvScreen*)screen;
	static const cc_string title = String_FromConst("Survival Inventory");
	struct DrawTextArgs args;

	Gui_MakeBodyFont(&s->font);
	Screen_UpdateVb(s);

	DrawTextArgs_Make(&args, &title, &s->font, true);
	Drawer2D_MakeTextTexture(&s->titleTex, &args);
}

static void SvInv_Update(void* screen, float delta) {
	struct SvInvScreen* s = (struct SvInvScreen*)screen;
	(void)delta;
	s->dirty = true;
}

static int SvInv_InputDown(void* screen, int key, struct InputDevice* device) {
	/* Consume the inventory key so ChatScreen can't re-open InventoryScreen */
	if (InputBind_Claims(BIND_INVENTORY, key, device)) return true;
	return Screen_InputDown(screen, key, device);
}

static int SvInv_PointerDown(void* screen, int id, int x, int y) {
	struct SvInvScreen* s = (struct SvInvScreen*)screen;
	struct SurvivalSlot tmp;
	int i, sx, sy;
	(void)id;

	for (i = 0; i < SURINV_TOTAL_SLOTS; i++) {
		SvInv_GetSlotPos(s, i, &sx, &sy);
		if (!Gui_Contains(sx, sy, SI_SLOT, SI_SLOT, x, y)) continue;

		if (s->heldSlot == -1) {
			if (SurvivalInv_Slots[i].id != BLOCK_AIR && SurvivalInv_Slots[i].count > 0)
				s->heldSlot = i;
		} else if (s->heldSlot == i) {
			s->heldSlot = -1;
		} else {
			/* Swap held slot with clicked slot */
			tmp = SurvivalInv_Slots[s->heldSlot];
			SurvivalInv_Slots[s->heldSlot] = SurvivalInv_Slots[i];
			SurvivalInv_Slots[i] = tmp;
			SurvivalInv_SyncHotbar();
			s->heldSlot = -1;
		}
		s->dirty = true;
		return true;
	}

	/* Click outside panel: cancel held slot */
	if (s->heldSlot != -1) { s->heldSlot = -1; s->dirty = true; }
	return false;
}

static int SvInv_MouseScroll(void* screen, float delta) { (void)screen; (void)delta; return true; }
static int SvInv_PointerMove(void* screen, int id, int x, int y) { (void)screen; (void)id; (void)x; (void)y; return false; }

static const struct ScreenVTABLE SvInv_VTABLE = {
	SvInv_Init,          SvInv_Update,       SvInv_Free,
	SvInv_Render,        SvInv_BuildMesh,
	SvInv_InputDown,     Screen_InputUp,     Screen_FKeyPress,  Screen_FText,
	SvInv_PointerDown,   Screen_PointerUp,   SvInv_PointerMove, SvInv_MouseScroll,
	SvInv_Layout,        SvInv_ContextLost,  SvInv_ContextRecreated,
	NULL /* HandlesPadAxis */
};

void SurvivalInv_Show(void) {
	struct SvInvScreen* s = &SvInvScreen_Instance;
	s->VTABLE = &SvInv_VTABLE;
	sv_inv_shown = true;
	Gui_Add((struct Screen*)s, GUI_PRIORITY_INVENTORY + 1);
}

void SurvivalInv_Hide(void) {
	sv_inv_shown = false;
	Gui_Remove((struct Screen*)&SvInvScreen_Instance);
}

/*########################################################################################################################*
*--------------------------------------------------Input key handler-----------------------------------------------------*
*#########################################################################################################################*/
static void SvInv_OnInventoryKey(void* obj, int key, cc_bool was, struct InputDevice* device) {
	(void)obj;
	if (was) return;
	if (!Survival_Active || !Server.IsSinglePlayer) return;
	if (!InputBind_Claims(BIND_INVENTORY, key, device)) return;

	if (sv_inv_shown) {
		SurvivalInv_Hide();
	} else {
		InventoryScreen_Hide();
		SurvivalInv_Show();
	}
}

/*########################################################################################################################*
*--------------------------------------------------Component interface---------------------------------------------------*
*#########################################################################################################################*/
static void SurvivalInv_Init(void) {
	Event_Register_(&InputEvents.Down2, NULL, SvInv_OnInventoryKey);
}

static void SurvivalInv_Free(void) {
	Event_Unregister_(&InputEvents.Down2, NULL, SvInv_OnInventoryKey);
	if (sv_inv_shown) SurvivalInv_Hide();
}

static void SurvivalInv_OnReset(void) {
	/* Just wipe the data; the Survival component owns (re)populating the */
	/* hotbar on map load, so it stays the single source of truth */
	SurvivalInv_Clear();
}

struct IGameComponent SurvivalInv_Component = {
	SurvivalInv_Init,    /* Init           */
	SurvivalInv_Free,    /* Free           */
	SurvivalInv_OnReset, /* Reset          */
	NULL,                /* OnNewMap       */
	NULL                 /* OnNewMapLoaded */
};
