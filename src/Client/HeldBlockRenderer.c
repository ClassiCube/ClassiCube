#include "HeldBlockRenderer.h"
#include "Block.h"
#include "Game.h"
#include "Inventory.h"
#include "GraphicsAPI.h"
#include "GraphicsCommon.h"
#include "Camera.h"
#include "ModelCache.h"
#include "ExtMath.h"
#include "Event.h"
#include "Entity.h"

BlockID held_block;
Entity held_entity;
EntityVTABLE held_entityVTABLE;
Matrix held_blockProjection;

bool held_animating, held_breaking, held_swinging;
Real32 held_swingY;
Real64 held_time, held_period = 0.25;
BlockID held_lastBlock;

void HeldBlockRenderer_RenderModel(void) {
	Gfx_SetFaceCulling(true);
	Gfx_SetTexturing(true);
	GfxCommon_SetupAlphaState(Block_Draw[held_block]);
	Gfx_SetDepthTest(false);

	IModel* model;
	if (Block_Draw[held_block] == DRAW_GAS) {
		String name = String_FromConst("arm"); model = ModelCache_Get(&name);
		held_entity.ModelScale = Vector3_Create1(1.0f);
	} else {
		String name = String_FromConst("block"); model = ModelCache_Get(&name);
		held_entity.ModelScale = Vector3_Create1(0.4f);
	}
	IModel_Render(model, &held_entity);

	Gfx_SetTexturing(false);
	GfxCommon_RestoreAlphaState(Block_Draw[held_block]);
	Gfx_SetDepthTest(true);
	Gfx_SetFaceCulling(false);
}

void HeldBlockRenderer_SetMatrix(void) {
	Entity* player = &LocalPlayer_Instance.Base;	
	Vector3 eyePos = Entity_GetEyePosition(player);
	Vector3 up = Vector3_UnitY;
	Vector3 target = eyePos; target.Z -= 1.0f; /* Look straight down*/	

	Matrix m, lookAt;
	Matrix_LookAt(&lookAt, eyePos, target, up);
	Matrix_Mul(&m, &lookAt, &Camera_TiltM);
	Gfx_View = m;
}

void HeldBlockRenderer_ResetHeldState(void) {
	/* Based off details from http://pastebin.com/KFV0HkmD (Thanks goodlyay!) */
	Entity* player = &LocalPlayer_Instance.Base;
	held_entity.Position = Entity_GetEyePosition(player);

	held_entity.Position.X -= Camera_BobbingHor;
	held_entity.Position.Y -= Camera_BobbingVer;
	held_entity.Position.Z -= Camera_BobbingHor;

	held_entity.HeadY = -45.0f; held_entity.RotY = -45.0f;
	held_entity.HeadX = 0.0f;   held_entity.RotX = 0.0f;
	held_entity.ModelBlock = held_block;
	held_entity.SkinType   = player->SkinType;
	held_entity.TextureId  = player->TextureId;
	held_entity.uScale     = player->uScale;
	held_entity.vScale     = player->vScale;
}

void HeldBlockRenderer_SetBaseOffset(void) {
	bool sprite = Block_Draw[held_block] == DRAW_SPRITE;
	Vector3 normalOffset = { 0.56f, -0.72f, -0.72f };
	Vector3 spriteOffset = { 0.46f, -0.52f, -0.72f };
	Vector3 offset = sprite ? spriteOffset : normalOffset;

	Vector3_Add(&held_entity.Position, &held_entity.Position, &offset);
	if (!sprite && Block_Draw[held_block] != DRAW_GAS) {
		Real32 height = Block_MaxBB[held_block].Y - Block_MinBB[held_block].Y;
		held_entity.Position.Y += 0.2f * (1.0f - height);
	}
}

void HeldBlockRenderer_ProjectionChanged(void* obj) {
	Real32 fov = 70.0f * MATH_DEG2RAD;
	Real32 aspectRatio = (Real32)Game_Width / (Real32)Game_Height;
	Real32 zNear = Gfx_MinZNear;
	Gfx_CalcPerspectiveMatrix(fov, aspectRatio, zNear, Game_ViewDistance, &held_blockProjection);
}

/* Based off incredible gifs from (Thanks goodlyay!)
	https://dl.dropboxusercontent.com/s/iuazpmpnr89zdgb/slowBreakTranslate.gif
	https://dl.dropboxusercontent.com/s/z7z8bset914s0ij/slowBreakRotate1.gif
	https://dl.dropboxusercontent.com/s/pdq79gkzntquld1/slowBreakRotate2.gif
	https://dl.dropboxusercontent.com/s/w1ego7cy7e5nrk1/slowBreakFull.gif

	https://github.com/UnknownShadow200/ClassicalSharp/wiki/Dig-animation-details
*/
void HeldBlockRenderer_DigAnimation(void) {
	Real32 t = held_time / held_period;
	Real32 sinHalfCircle = Math_Sin(t * MATH_PI);
	Real32 sqrtLerpPI = Math_Sqrt(t) * MATH_PI;

	held_entity.Position.X -= Math_Sin(sqrtLerpPI)       * 0.4f;
	held_entity.Position.Y += Math_Sin((sqrtLerpPI * 2)) * 0.2f;
	held_entity.Position.Z -= sinHalfCircle              * 0.2f;

	Real32 sinHalfCircleWeird = Math_Sin(t * t * MATH_PI);
	held_entity.RotY -= Math_Sin(sqrtLerpPI)  * 80.0f;
	held_entity.HeadY -= Math_Sin(sqrtLerpPI) * 80.0f;
	held_entity.RotX += sinHalfCircleWeird    * 20.0f;
}

void HeldBlockRenderer_ResetAnim(bool setLastHeld, Real64 period) {
	held_time = 0.0f; held_swingY = 0.0f;
	held_animating = false; held_swinging = false;
	held_period = period;
	if (setLastHeld) { held_lastBlock = Inventory_SelectedBlock; }
}

PackedCol HeldBlockRenderer_GetCol(Entity* entity) {
	Entity* player = &LocalPlayer_Instance.Base;
	PackedCol col = player->VTABLE->GetCol(player);

	/* Adjust pitch so angle when looking straight down is 0. */
	Real32 adjHeadX = player->HeadX - 90.0f;
	if (adjHeadX < 0.0f) adjHeadX += 360.0f;

	/* Adjust colour so held block is brighter when looking straight up */
	Real32 t = Math_AbsF(adjHeadX - 180.0f) / 180.0f;
	Real32 colScale = Math_Lerp(0.9f, 0.7f, t);
	return PackedCol_Scale(col, colScale);
}

void HeldBlockRenderer_ClickAnim(bool digging) {
	/* TODO: timing still not quite right, rotate2 still not quite right */
	HeldBlockRenderer_ResetAnim(true, digging ? 0.35 : 0.25);
	held_swinging = false;
	held_breaking = digging;
	held_animating = true;
	/* Start place animation at bottom of cycle */
	if (!digging) held_time = held_period / 2;
}

void HeldBlockRenderer_DoSwitchBlockAnim(void* obj) {
	if (held_swinging) {
		/* Like graph -sin(x) : x=0.5 and x=2.5 have same y values,
		   but increasing x causes y to change in opposite directions */
		if (held_time > held_period * 0.5f) {
			held_time = held_period - held_time;
		}
	} else {
		if (held_block == Inventory_SelectedBlock) return;
		HeldBlockRenderer_ResetAnim(false, 0.25);
		held_animating = true;
		held_swinging = true;
	}
}

void HeldBlockRenderer_BlockChanged(void* obj, Vector3I coords, BlockID oldBlock, BlockID block) {
	if (block == BLOCK_AIR) return;
	HeldBlockRenderer_ClickAnim(false);
}

void HeldBlockRenderer_DoAnimation(Real64 delta, Real32 lastSwingY) {
	if (!held_animating) return;

	if (held_swinging || !held_breaking) {
		Real32 t = held_time / held_period;
		held_swingY = -0.4f * Math_Sin(t * MATH_PI);
		held_entity.Position.Y += held_swingY;

		if (held_swinging) {
			/* i.e. the block has gone to bottom of screen and is now returning back up. 
			   At this point we switch over to the new held block. */
			if (held_swingY > lastSwingY) held_lastBlock = held_block;
			held_block = held_lastBlock;
			held_entity.ModelBlock = held_block;
		}
	} else {
		HeldBlockRenderer_DigAnimation();
	}
	
	held_time += delta;
	if (held_time > held_period) {
		HeldBlockRenderer_ResetAnim(true, 0.25);
	}
}

void HeldBlockRenderer_Render(Real64 delta) {
	if (!Game_ShowBlockInHand) return;

	Real32 lastSwingY = held_swingY;
	held_swingY = 0.0f;
	held_block = Inventory_SelectedBlock;

	Gfx_SetMatrixMode(MATRIX_TYPE_PROJECTION);
	Gfx_LoadMatrix(&held_blockProjection);
	Gfx_SetMatrixMode(MATRIX_TYPE_MODELVIEW);
	Matrix view = Gfx_View;
	HeldBlockRenderer_SetMatrix();

	HeldBlockRenderer_ResetHeldState();
	HeldBlockRenderer_DoAnimation(delta, lastSwingY);
	HeldBlockRenderer_SetBaseOffset();
	if (!Camera_ActiveCamera->IsThirdPerson) HeldBlockRenderer_RenderModel();

	Gfx_View = view;
	Gfx_SetMatrixMode(MATRIX_TYPE_PROJECTION);
	Gfx_LoadMatrix(&Gfx_Projection);
	Gfx_SetMatrixMode(MATRIX_TYPE_MODELVIEW);
}

void HeldBlockRenderer_Init(void) {
	Entity_Init(&held_entity);
	held_entityVTABLE = *held_entity.VTABLE;
	held_entityVTABLE.GetCol = HeldBlockRenderer_GetCol;
	held_entity.VTABLE = &held_entityVTABLE;

	held_lastBlock = Inventory_SelectedBlock;
	Event_RegisterVoid(&GfxEvents_ProjectionChanged, NULL, HeldBlockRenderer_ProjectionChanged);
	Event_RegisterVoid(&UserEvents_HeldBlockChanged, NULL, HeldBlockRenderer_DoSwitchBlockAnim);
	Event_RegisterBlock(&UserEvents_BlockChanged,    NULL, HeldBlockRenderer_BlockChanged);
}

void HeldBlockRenderer_Free(void) {
	Event_UnregisterVoid(&GfxEvents_ProjectionChanged, NULL, HeldBlockRenderer_ProjectionChanged);
	Event_UnregisterVoid(&UserEvents_HeldBlockChanged, NULL, HeldBlockRenderer_DoSwitchBlockAnim);
	Event_UnregisterBlock(&UserEvents_BlockChanged,    NULL, HeldBlockRenderer_BlockChanged);
}

IGameComponent HeldBlockRenderer_MakeComponent(void) {
	IGameComponent comp = IGameComponent_MakeEmpty();
	comp.Init = HeldBlockRenderer_Init;
	comp.Free = HeldBlockRenderer_Free;
	return comp;
}