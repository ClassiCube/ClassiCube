#include "HeldBlockRenderer.h"
#include "Block.h"
#include "Game.h"
#include "Inventory.h"
#include "Graphics.h"
#include "Camera.h"
#include "ExtMath.h"
#include "Event.h"
#include "Entity.h"
#include "Model.h"
#include "Options.h"

cc_bool HeldBlockRenderer_Show;
static BlockID held_block;
static struct Entity held_entity;
static struct Matrix held_blockProj;

static cc_bool held_animating, held_breaking, held_swinging;
static float held_swingY;
static double held_time, held_period = 0.25;
static BlockID held_lastBlock;

static void HeldBlockRenderer_RenderModel(void) {
	static const cc_string block = String_FromConst("block");
	struct Model* model;

	Gfx_SetFaceCulling(true);
	Gfx_SetDepthTest(false);
	/* TODO: Need to properly reallocate per model VB here */

	if (Blocks.Draw[held_block] == DRAW_GAS) {
		model = LocalPlayer_Instance.Base.Model;
		Vec3_Set(held_entity.ModelScale, 1.0f,1.0f,1.0f);

		Gfx_SetAlphaTest(true);
		Model_RenderArm(model, &held_entity);
		Gfx_SetAlphaTest(false);
	} else {	
		model = Model_Get(&block);
		Vec3_Set(held_entity.ModelScale, 0.4f,0.4f,0.4f);

		Gfx_SetupAlphaState(Blocks.Draw[held_block]);
		Model_Render(model, &held_entity);
		Gfx_RestoreAlphaState(Blocks.Draw[held_block]);
	}
	
	Gfx_SetDepthTest(true);
	Gfx_SetFaceCulling(false);
}

static void SetMatrix(void) {
	struct Entity* p = &LocalPlayer_Instance.Base;
	struct Matrix lookAt;
	Vec3 eye = { 0,0,0 }; eye.y = Entity_GetEyeHeight(p);

	Matrix_Translate(&lookAt, -eye.x, -eye.y, -eye.z);
	Matrix_Mul(&Gfx.View, &lookAt, &Camera.TiltM);
}

static void ResetHeldState(void) {
	/* Based off details from http://pastebin.com/KFV0HkmD (Thanks goodlyay!) */
	struct Entity* p = &LocalPlayer_Instance.Base;
	Vec3 eye = { 0,0,0 }; eye.y = Entity_GetEyeHeight(p);
	held_entity.Position = eye;

	held_entity.Position.x -= Camera.BobbingHor;
	held_entity.Position.y -= Camera.BobbingVer;
	held_entity.Position.z -= Camera.BobbingHor;

	held_entity.Yaw   = -45.0f; held_entity.RotY = -45.0f;
	held_entity.Pitch = 0.0f;   held_entity.RotX = 0.0f;
	held_entity.ModelBlock   = held_block;

	held_entity.SkinType     = p->SkinType;
	held_entity.TextureId    = p->TextureId;
	held_entity.MobTextureId = p->MobTextureId;
	held_entity.uScale       = p->uScale;
	held_entity.vScale       = p->vScale;
}

static void SetBaseOffset(void) {
	cc_bool sprite = Blocks.Draw[held_block] == DRAW_SPRITE;
	Vec3 normalOffset = { 0.56f, -0.72f, -0.72f };
	Vec3 spriteOffset = { 0.46f, -0.52f, -0.72f };
	Vec3 offset = sprite ? spriteOffset : normalOffset;

	Vec3_AddBy(&held_entity.Position, &offset);
	if (!sprite && Blocks.Draw[held_block] != DRAW_GAS) {
		float height = Blocks.MaxBB[held_block].y - Blocks.MinBB[held_block].y;
		held_entity.Position.y += 0.2f * (1.0f - height);
	}
}

static void OnProjectionChanged(void* obj) {
	float fov = 70.0f * MATH_DEG2RAD;
	float aspectRatio = (float)Game.Width / (float)Game.Height;
	Gfx_CalcPerspectiveMatrix(&held_blockProj, fov, aspectRatio, (float)Game_ViewDistance);
}

/* Based off incredible gifs from (Thanks goodlyay!)
	https://dl.dropboxusercontent.com/s/iuazpmpnr89zdgb/slowBreakTranslate.gif
	https://dl.dropboxusercontent.com/s/z7z8bset914s0ij/slowBreakRotate1.gif
	https://dl.dropboxusercontent.com/s/pdq79gkzntquld1/slowBreakRotate2.gif
	https://dl.dropboxusercontent.com/s/w1ego7cy7e5nrk1/slowBreakFull.gif

	https://github.com/UnknownShadow200/ClassicalSharp/wiki/Dig-animation-details
*/
static void HeldBlockRenderer_DigAnimation(void) {
	double sinHalfCircle, sinHalfCircleWeird;
	float t, sqrtLerpPI;

	t = held_time / held_period;
	sinHalfCircle = Math_Sin(t * MATH_PI);
	sqrtLerpPI    = Math_SqrtF(t) * MATH_PI;

	held_entity.Position.x -= (float)Math_Sin(sqrtLerpPI)     * 0.4f;
	held_entity.Position.y += (float)Math_Sin(sqrtLerpPI * 2) * 0.2f;
	held_entity.Position.z -= (float)sinHalfCircle            * 0.2f;

	sinHalfCircleWeird = Math_Sin(t * t * MATH_PI);
	held_entity.RotY  -= (float)Math_Sin(sqrtLerpPI) * 80.0f;
	held_entity.Yaw   -= (float)Math_Sin(sqrtLerpPI) * 80.0f;
	held_entity.RotX  += (float)sinHalfCircleWeird   * 20.0f;
}

static void HeldBlockRenderer_ResetAnim(cc_bool setLastHeld, double period) {
	held_time = 0.0f; held_swingY = 0.0f;
	held_animating = false; held_swinging = false;
	held_period = period;
	if (setLastHeld) { held_lastBlock = Inventory_SelectedBlock; }
}

static PackedCol HeldBlockRenderer_GetCol(struct Entity* entity) {
	struct Entity* player;
	PackedCol col;
	float adjPitch, t, scale;

	player = &LocalPlayer_Instance.Base;
	col    = player->VTABLE->GetCol(player);

	/* Adjust pitch so angle when looking straight down is 0. */
	adjPitch = player->Pitch - 90.0f;
	if (adjPitch < 0.0f) adjPitch += 360.0f;

	/* Adjust color so held block is brighter when looking straight up */
	t     = Math_AbsF(adjPitch - 180.0f) / 180.0f;
	scale = Math_Lerp(0.9f, 0.7f, t);
	return PackedCol_Scale(col, scale);
}

void HeldBlockRenderer_ClickAnim(cc_bool digging) {
	/* TODO: timing still not quite right, rotate2 still not quite right */
	HeldBlockRenderer_ResetAnim(true, digging ? 0.35 : 0.25);
	held_swinging  = false;
	held_breaking  = digging;
	held_animating = true;
	/* Start place animation at bottom of cycle */
	if (!digging) held_time = held_period / 2;
}

static void DoSwitchBlockAnim(void* obj) {
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

static void OnBlockChanged(void* obj, IVec3 coords, BlockID old, BlockID now) {
	if (now == BLOCK_AIR) return;
	HeldBlockRenderer_ClickAnim(false);
}

static void DoAnimation(double delta, float lastSwingY) {
	double t;
	if (!held_animating) return;

	if (held_swinging || !held_breaking) {
		t = held_time / held_period;
		held_swingY = -0.4f * (float)Math_Sin(t * MATH_PI);
		held_entity.Position.y += held_swingY;

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

void HeldBlockRenderer_Render(double delta) {
	float lastSwingY;
	struct Matrix view;
	if (!HeldBlockRenderer_Show) return;

	lastSwingY  = held_swingY;
	held_swingY = 0.0f;
	held_block  = Inventory_SelectedBlock;
	view = Gfx.View;

	Gfx_LoadMatrix(MATRIX_PROJECTION, &held_blockProj);
	SetMatrix();

	ResetHeldState();
	DoAnimation(delta, lastSwingY);
	SetBaseOffset();
	if (!Camera.Active->isThirdPerson) HeldBlockRenderer_RenderModel();

	Gfx.View = view;
	Gfx_LoadMatrix(MATRIX_PROJECTION, &Gfx.Projection);
}


static void OnContextLost(void* obj) {
	Gfx_DeleteDynamicVb(&held_entity.ModelVB);
}

static const struct EntityVTABLE heldEntity_VTABLE = {
	NULL, NULL, NULL, HeldBlockRenderer_GetCol,
	NULL, NULL
};
static void OnInit(void) {
	Entity_Init(&held_entity);
	held_entity.VTABLE  = &heldEntity_VTABLE;
	held_entity.NoShade = true;

	HeldBlockRenderer_Show = Options_GetBool(OPT_SHOW_BLOCK_IN_HAND, true);
	held_lastBlock         = Inventory_SelectedBlock;

	Event_Register_(&GfxEvents.ProjectionChanged, NULL, OnProjectionChanged);
	Event_Register_(&UserEvents.HeldBlockChanged, NULL, DoSwitchBlockAnim);
	Event_Register_(&UserEvents.BlockChanged,     NULL, OnBlockChanged);
	Event_Register_(&GfxEvents.ContextLost,       NULL, OnContextLost);
}

struct IGameComponent HeldBlockRenderer_Component = {
	OnInit /* Init  */
};
