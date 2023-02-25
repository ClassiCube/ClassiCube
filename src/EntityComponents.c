#include "EntityComponents.h"
#include "String.h"
#include "ExtMath.h"
#include "World.h"
#include "Block.h"
#include "Event.h"
#include "Game.h"
#include "Entity.h"
#include "Platform.h"
#include "Camera.h"
#include "Funcs.h"
#include "Graphics.h"
#include "Physics.h"
#include "Model.h"
#include "Audio.h"
#include "Bitmap.h"

/*########################################################################################################################*
*----------------------------------------------------AnimatedComponent----------------------------------------------------*
*#########################################################################################################################*/
#define ANIM_MAX_ANGLE (110 * MATH_DEG2RAD)
#define ANIM_ARM_MAX (60.0f * MATH_DEG2RAD)
#define ANIM_LEG_MAX (80.0f * MATH_DEG2RAD)
#define ANIM_IDLE_MAX (3.0f * MATH_DEG2RAD)
#define ANIM_IDLE_XPERIOD (2.0f * MATH_PI / 5.0f)
#define ANIM_IDLE_ZPERIOD (2.0f * MATH_PI / 3.5f)

static void AnimatedComp_DoTilt(float* tilt, cc_bool reduce) {
	if (reduce) {
		(*tilt) *= 0.84f;
	} else {
		(*tilt) += 0.1f;
	}
	Math_Clamp(*tilt, 0.0f, 1.0f);
}

static void AnimatedComp_PerpendicularAnim(struct AnimatedComp* anim, float flapSpeed, float idleXRot, float idleZRot, cc_bool left) {
	float verAngle = 0.5f + 0.5f * Math_SinF(anim->WalkTime * flapSpeed);
	float horAngle = Math_CosF(anim->WalkTime);
	float zRot = -idleZRot - verAngle * anim->Swing * ANIM_MAX_ANGLE;	
	float xRot =  idleXRot + horAngle * anim->Swing * ANIM_ARM_MAX * 1.5f;

	if (left) {
		anim->LeftArmX  = xRot; anim->LeftArmZ  = zRot;
	} else {
		anim->RightArmX = xRot; anim->RightArmZ = zRot;
	}
}

static void AnimatedComp_CalcHumanAnim(struct AnimatedComp* anim, float idleXRot, float idleZRot) {
	AnimatedComp_PerpendicularAnim(anim, 0.23f, idleXRot, idleZRot, true);
	AnimatedComp_PerpendicularAnim(anim, 0.28f, idleXRot, idleZRot, false);
	anim->RightArmX = -anim->RightArmX; anim->RightArmZ = -anim->RightArmZ;
}

void AnimatedComp_Init(struct AnimatedComp* anim) {
	Mem_Set(anim, 0, sizeof(struct AnimatedComp));
	anim->BobStrength = 1.0f; anim->BobStrengthO = 1.0f; anim->BobStrengthN = 1.0f;
}

void AnimatedComp_Update(struct Entity* e, Vec3 oldPos, Vec3 newPos, double delta) {
	struct AnimatedComp* anim = &e->Anim;
	float dx = newPos.X - oldPos.X;
	float dz = newPos.Z - oldPos.Z;
	float distance = Math_SqrtF(dx * dx + dz * dz);
	int i;

	float walkDelta;
	anim->WalkTimeO = anim->WalkTimeN;
	anim->SwingO    = anim->SwingN;

	if (distance > 0.05f) {
		walkDelta = distance * 2 * (float)(20 * delta);
		anim->WalkTimeN += walkDelta;
		anim->SwingN += (float)delta * 3;
	} else {
		anim->SwingN -= (float)delta * 3;
	}
	Math_Clamp(anim->SwingN, 0.0f, 1.0f);

	/* TODO: the Tilt code was designed for 60 ticks/second, fix it up for 20 ticks/second */
	anim->BobStrengthO = anim->BobStrengthN;
	for (i = 0; i < 3; i++) {
		AnimatedComp_DoTilt(&anim->BobStrengthN, !Game_ViewBobbing || !e->OnGround);
	}
}

void AnimatedComp_GetCurrent(struct Entity* e, float t) {
	struct AnimatedComp* anim = &e->Anim;
	float idleTime = (float)Game.Time;
	float idleXRot = Math_SinF(idleTime * ANIM_IDLE_XPERIOD) * ANIM_IDLE_MAX;
	float idleZRot = Math_CosF(idleTime * ANIM_IDLE_ZPERIOD) * ANIM_IDLE_MAX + ANIM_IDLE_MAX;

	anim->Swing       = Math_Lerp(anim->SwingO,       anim->SwingN,       t);
	anim->WalkTime    = Math_Lerp(anim->WalkTimeO,    anim->WalkTimeN,    t);
	anim->BobStrength = Math_Lerp(anim->BobStrengthO, anim->BobStrengthN, t);

	anim->LeftArmX =  (Math_CosF(anim->WalkTime) * anim->Swing * ANIM_ARM_MAX) - idleXRot;
	anim->LeftArmZ = -idleZRot;
	anim->LeftLegX = -(Math_CosF(anim->WalkTime) * anim->Swing * ANIM_LEG_MAX);
	anim->LeftLegZ = 0;

	anim->RightLegX = -anim->LeftLegX; anim->RightLegZ = -anim->LeftLegZ;
	anim->RightArmX = -anim->LeftArmX; anim->RightArmZ = -anim->LeftArmZ;

	anim->BobbingHor   = Math_CosF(anim->WalkTime)            * anim->Swing * (2.5f/16.0f);
	anim->BobbingVer   = Math_AbsF(Math_SinF(anim->WalkTime)) * anim->Swing * (2.5f/16.0f);
	anim->BobbingModel = Math_AbsF(Math_CosF(anim->WalkTime)) * anim->Swing * (4.0f/16.0f);

	if (e->Model->calcHumanAnims && !Game_SimpleArmsAnim) {
		AnimatedComp_CalcHumanAnim(anim, idleXRot, idleZRot);
	}
}


/*########################################################################################################################*
*------------------------------------------------------TiltComponent------------------------------------------------------*
*#########################################################################################################################*/
void TiltComp_Init(struct TiltComp* anim) {
	anim->TiltX = 0.0f; anim->TiltY = 0.0f; anim->VelTiltStrength = 1.0f;
	anim->VelTiltStrengthO = 1.0f; anim->VelTiltStrengthN = 1.0f;
}

void TiltComp_Update(struct TiltComp* anim, double delta) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	int i;

	anim->VelTiltStrengthO = anim->VelTiltStrengthN;
	/* TODO: the Tilt code was designed for 60 ticks/second, fix it up for 20 ticks/second */
	for (i = 0; i < 3; i++) {
		AnimatedComp_DoTilt(&anim->VelTiltStrengthN, p->Hacks.Floating);
	}
}

void TiltComp_GetCurrent(struct TiltComp* anim, float t) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	struct AnimatedComp* pAnim = &p->Base.Anim;

	anim->VelTiltStrength = Math_Lerp(anim->VelTiltStrengthO, anim->VelTiltStrengthN, t);
	anim->TiltX = Math_CosF(pAnim->WalkTime) * pAnim->Swing * (0.15f * MATH_DEG2RAD);
	anim->TiltY = Math_SinF(pAnim->WalkTime) * pAnim->Swing * (0.15f * MATH_DEG2RAD);
}


/*########################################################################################################################*
*-----------------------------------------------------HacksComponent------------------------------------------------------*
*#########################################################################################################################*/
static void HacksComp_SetAll(struct HacksComp* hacks, cc_bool allowed) {
	hacks->CanAnyHacks = allowed; hacks->CanFly            = allowed;
	hacks->CanNoclip   = allowed; hacks->CanRespawn        = allowed;
	hacks->CanSpeed    = allowed; hacks->CanPushbackBlocks = allowed;

	hacks->CanUseThirdPerson = allowed;
	hacks->CanSeeAllNames    = allowed && hacks->IsOp;
}

void HacksComp_Init(struct HacksComp* hacks) {
	Mem_Set(hacks, 0, sizeof(struct HacksComp));
	HacksComp_SetAll(hacks, true);
	hacks->SpeedMultiplier = 10.0f;
	hacks->Enabled = true;
	hacks->IsOp           = true;
	hacks->CanSeeAllNames = true;
	hacks->CanDoubleJump  = true;
	hacks->BaseHorSpeed   = 1.0f;
	hacks->MaxHorSpeed    = 1.0f;
	hacks->MaxJumps       = 1;
	hacks->NoclipSlide    = true;
	hacks->CanBePushed    = true;

	String_InitArray(hacks->HacksFlags, hacks->__HacksFlagsBuffer);
}

cc_bool HacksComp_CanJumpHigher(struct HacksComp* hacks) {
	return hacks->Enabled && hacks->CanSpeed;
}

static cc_string HacksComp_UNSAFE_FlagValue(const char* flag, struct HacksComp* hacks) {
	cc_string* joined = &hacks->HacksFlags;
	int beg, end;

	beg = String_IndexOfConst(joined, flag);
	if (beg < 0) return String_Empty;
	beg += String_Length(flag);

	end = String_IndexOfAt(joined, beg, ' ');
	if (end < 0) end = joined->length;

	return String_UNSAFE_Substring(joined, beg, end - beg);
}

static float HacksComp_ParseFlagFloat(const char* flagRaw, struct HacksComp* hacks) {
	cc_string raw = HacksComp_UNSAFE_FlagValue(flagRaw, hacks);
	float value;

	if (!raw.length || Game_ClassicMode)   return 1.0f;
	if (!Convert_ParseFloat(&raw, &value)) return 1.0f;
	return value;
}

static int HacksComp_ParseFlagInt(const char* flagRaw, struct HacksComp* hacks) {
	cc_string raw = HacksComp_UNSAFE_FlagValue(flagRaw, hacks);
	int value;

	if (!raw.length || Game_ClassicMode) return 1;
	if (!Convert_ParseInt(&raw, &value)) return 1;
	return value;
}

static void HacksComp_ParseFlag(struct HacksComp* hacks, const char* include, const char* exclude, cc_bool* target) {
	cc_string* joined = &hacks->HacksFlags;
	if (String_ContainsConst(joined, include)) {
		*target = true;
	} else if (String_ContainsConst(joined, exclude)) {
		*target = false;
	}
}

static void HacksComp_ParseAllFlag(struct HacksComp* hacks, const char* include, const char* exclude) {
	cc_string* joined = &hacks->HacksFlags;
	if (String_ContainsConst(joined, include)) {
		HacksComp_SetAll(hacks, true);
	} else if (String_ContainsConst(joined, exclude)) {
		HacksComp_SetAll(hacks, false);
	}
}

void HacksComp_RecheckFlags(struct HacksComp* hacks) {
	/* Can use hacks by default (also case with WoM), no need to check +hax */
	cc_bool hax = !String_ContainsConst(&hacks->HacksFlags, "-hax");
	HacksComp_SetAll(hacks, hax);
	hacks->CanBePushed   = true;

	HacksComp_ParseFlag(hacks, "+fly",         "-fly",         &hacks->CanFly);
	HacksComp_ParseFlag(hacks, "+noclip",      "-noclip",      &hacks->CanNoclip);
	HacksComp_ParseFlag(hacks, "+speed",       "-speed",       &hacks->CanSpeed);
	HacksComp_ParseFlag(hacks, "+respawn",     "-respawn",     &hacks->CanRespawn);
	HacksComp_ParseFlag(hacks, "+push",        "-push",        &hacks->CanBePushed);
	HacksComp_ParseFlag(hacks, "+thirdperson", "-thirdperson", &hacks->CanUseThirdPerson);
	HacksComp_ParseFlag(hacks, "+names",       "-names",       &hacks->CanSeeAllNames);

	if (hacks->IsOp) HacksComp_ParseAllFlag(hacks, "+ophax", "-ophax");
	hacks->BaseHorSpeed = HacksComp_ParseFlagFloat("horspeed=", hacks);
	hacks->MaxHorSpeed  = HacksComp_ParseFlagFloat("maxspeed=", hacks);
	hacks->MaxJumps     = HacksComp_ParseFlagInt("jumps=",      hacks);
	HacksComp_Update(hacks);
}

void HacksComp_Update(struct HacksComp* hacks) {
	if (!hacks->CanFly || !hacks->Enabled) {
		HacksComp_SetFlying(hacks, false); 
		hacks->FlyingDown = false; hacks->FlyingUp = false;
	}
	if (!hacks->CanNoclip || !hacks->Enabled) {
		HacksComp_SetNoclip(hacks, false);
	}
	if (!hacks->CanSpeed || !hacks->Enabled) {
		hacks->Speeding = false; hacks->HalfSpeeding = false;
	}

	hacks->CanDoubleJump = hacks->Enabled && hacks->CanSpeed;
	Event_RaiseVoid(&UserEvents.HackPermsChanged);
}

void HacksComp_SetFlying(struct HacksComp* hacks, cc_bool flying) {
	if (hacks->Flying == flying) return;
	hacks->Flying = flying;
	Event_RaiseVoid(&UserEvents.HacksStateChanged);
}

void HacksComp_SetNoclip(struct HacksComp* hacks, cc_bool noclip) {
	if (hacks->Noclip == noclip) return;
	hacks->Noclip = noclip;
	Event_RaiseVoid(&UserEvents.HacksStateChanged);
}

float HacksComp_CalcSpeedFactor(struct HacksComp* hacks, cc_bool canSpeed) {
	float speed = 0;
	if (!canSpeed) return 0;

	if (hacks->HalfSpeeding) speed += hacks->SpeedMultiplier / 2;
	if (hacks->Speeding)     speed += hacks->SpeedMultiplier;
	return speed;
}


/*########################################################################################################################*
*--------------------------------------------------InterpolationComponent-------------------------------------------------*
*#########################################################################################################################*/
static void InterpComp_RemoveOldestRotY(struct InterpComp* interp) {
	int i;
	for (i = 0; i < Array_Elems(interp->RotYStates); i++) {
		interp->RotYStates[i] = interp->RotYStates[i + 1];
	}
	interp->RotYCount--;
}

static void InterpComp_AddRotY(struct InterpComp* interp, float state) {
	if (interp->RotYCount == Array_Elems(interp->RotYStates)) {
		InterpComp_RemoveOldestRotY(interp);
	}
	interp->RotYStates[interp->RotYCount] = state; interp->RotYCount++;
}

static void InterpComp_AdvanceRotY(struct InterpComp* interp, struct Entity* e) {
	if (!interp->RotYCount) return;

	e->next.rotY = interp->RotYStates[0];
	InterpComp_RemoveOldestRotY(interp);
}


/*########################################################################################################################*
*----------------------------------------------NetworkInterpolationComponent----------------------------------------------*
*#########################################################################################################################*/
#define NetInterpAngles_Copy(dst, src) \
(dst).pitch = (src)->Pitch;\
(dst).yaw   = (src)->Yaw;\
(dst).rotX  = (src)->RotX;\
(dst).rotZ  = (src)->RotZ;

static void NetInterpComp_RemoveOldestPosition(struct NetInterpComp* interp) {
	int i;
	interp->PositionsCount--;

	for (i = 0; i < interp->PositionsCount; i++) {
		interp->Positions[i] = interp->Positions[i + 1];
	}
}

static void NetInterpComp_AddPosition(struct NetInterpComp* interp, Vec3 pos) {
	if (interp->PositionsCount == Array_Elems(interp->Positions)) {
		NetInterpComp_RemoveOldestPosition(interp);
	}
	interp->Positions[interp->PositionsCount++] = pos;
}

static void NetInterpComp_SetPosition(struct NetInterpComp* interp, struct LocationUpdate* update, struct Entity* e, int mode) {
	Vec3 lastPos = interp->CurPos;
	Vec3* curPos = &interp->CurPos;
	Vec3 midPos;

	if (mode == LU_POS_ABSOLUTE_INSTANT || mode == LU_POS_ABSOLUTE_SMOOTH) {
		*curPos = update->pos;
	} else {
		Vec3_AddBy(curPos, &update->pos);
	}

	if (mode == LU_POS_ABSOLUTE_INSTANT) {
		e->prev.pos = *curPos;
		e->next.pos = *curPos;
		interp->PositionsCount = 0;
	} else {
		/* Smoother interpolation by also adding midpoint */
		Vec3_Lerp(&midPos, &lastPos, curPos, 0.5f);
		NetInterpComp_AddPosition(interp,  midPos);
		NetInterpComp_AddPosition(interp, *curPos);
	}
}

static void NetInterpComp_RemoveOldestAngles(struct NetInterpComp* interp) {
	int i;
	interp->AnglesCount--;

	for (i = 0; i < interp->AnglesCount; i++) {
		interp->Angles[i] = interp->Angles[i + 1];
	}
}

static void NetInterpComp_AddAngles(struct NetInterpComp* interp, struct NetInterpAngles angles) {
	if (interp->AnglesCount == Array_Elems(interp->Angles)) {
		NetInterpComp_RemoveOldestAngles(interp);
	}
	interp->Angles[interp->AnglesCount++] = angles;
}

void NetInterpComp_SetLocation(struct NetInterpComp* interp, struct LocationUpdate* update, struct Entity* e) {
	struct NetInterpAngles last = interp->CurAngles;
	struct NetInterpAngles* cur = &interp->CurAngles;
	struct NetInterpAngles mid;
	cc_uint8 flags      = update->flags;
	cc_bool interpolate = flags & LU_ORI_INTERPOLATE;

	if (flags & LU_HAS_POS) {
		NetInterpComp_SetPosition(interp, update, e, flags & LU_POS_MODEMASK);
	}
	if (flags & LU_HAS_ROTX)  cur->RotX  = Math_ClampAngle(update->rotX);
	if (flags & LU_HAS_ROTZ)  cur->RotZ  = Math_ClampAngle(update->rotZ);
	if (flags & LU_HAS_PITCH) cur->Pitch = Math_ClampAngle(update->pitch);
	if (flags & LU_HAS_YAW)   cur->Yaw   = Math_ClampAngle(update->yaw);

	if (!interpolate) {
		NetInterpAngles_Copy(e->prev, cur); e->prev.rotY = cur->Yaw;
		NetInterpAngles_Copy(e->next, cur); e->next.rotY = cur->Yaw;
		interp->RotYCount = 0; interp->AnglesCount = 0;
	} else {
		/* Smoother interpolation by also adding midpoint */
		mid.RotX  = Math_LerpAngle(last.RotX,  cur->RotX,  0.5f);
		mid.RotZ  = Math_LerpAngle(last.RotZ,  cur->RotZ,  0.5f);
		mid.Pitch = Math_LerpAngle(last.Pitch, cur->Pitch, 0.5f);
		mid.Yaw   = Math_LerpAngle(last.Yaw,   cur->Yaw,   0.5f);
		NetInterpComp_AddAngles(interp, mid);
		NetInterpComp_AddAngles(interp, *cur);

		/* Body rotation lags behind head a tiny bit */
		InterpComp_AddRotY((struct InterpComp*)interp, Math_LerpAngle(last.Yaw, cur->Yaw, 0.33333333f));
		InterpComp_AddRotY((struct InterpComp*)interp, Math_LerpAngle(last.Yaw, cur->Yaw, 0.66666667f));
		InterpComp_AddRotY((struct InterpComp*)interp, Math_LerpAngle(last.Yaw, cur->Yaw, 1.00000000f));
	}
}

void NetInterpComp_AdvanceState(struct NetInterpComp* interp, struct Entity* e) {
	e->prev     = e->next;
	e->Position = e->prev.pos;

	if (interp->PositionsCount) {
		e->next.pos = interp->Positions[0];
		NetInterpComp_RemoveOldestPosition(interp);
	}
	if (interp->AnglesCount) {
		NetInterpAngles_Copy(e->next, &interp->Angles[0]);
		NetInterpComp_RemoveOldestAngles(interp);
	}
	InterpComp_AdvanceRotY((struct InterpComp*)interp, e);
}


/*########################################################################################################################*
*-----------------------------------------------LocalInterpolationComponent-----------------------------------------------*
*#########################################################################################################################*/
static void LocalInterpComp_SetPosition(struct LocationUpdate* update, int mode) {
	struct Entity* e = &LocalPlayer_Instance.Base;
	float yOffset;

	if (mode == LU_POS_ABSOLUTE_INSTANT || mode == LU_POS_ABSOLUTE_SMOOTH) {
		e->next.pos = update->pos;
	} else if (mode == LU_POS_RELATIVE_SMOOTH) {
		Vec3_AddBy(&e->next.pos, &update->pos);
	} else if (mode == LU_POS_RELATIVE_SHIFT) {
		Vec3_AddBy(&e->prev.pos, &update->pos);
		Vec3_AddBy(&e->next.pos, &update->pos);
	}

	/* If server sets Y position exactly on ground, push up a tiny bit */
	yOffset = e->next.pos.Y - Math_Floor(e->next.pos.Y);
	if (yOffset < ENTITY_ADJUSTMENT) e->next.pos.Y += ENTITY_ADJUSTMENT;

	if (mode == LU_POS_ABSOLUTE_INSTANT) { 
		e->prev.pos = e->next.pos; e->Position = e->next.pos; 
	}
}

static void LocalInterpComp_Angle(float* prev, float* next, float value, cc_bool interpolate) {
	value = Math_ClampAngle(value);
	*next = value;
	if (!interpolate) *prev = value;
}

void LocalInterpComp_SetLocation(struct InterpComp* interp, struct LocationUpdate* update) {
	struct Entity* e = &LocalPlayer_Instance.Base;
	struct EntityLocation* prev = &e->prev;
	struct EntityLocation* next = &e->next;
	cc_uint8 flags      = update->flags;
	cc_bool interpolate = flags & LU_ORI_INTERPOLATE;

	if (flags & LU_HAS_POS) {
		LocalInterpComp_SetPosition(update, flags & LU_POS_MODEMASK);
	}
	if (flags & LU_HAS_PITCH) {
		LocalInterpComp_Angle(&prev->pitch, &next->pitch, update->pitch, interpolate);
	}
	if (flags & LU_HAS_ROTX) {
		LocalInterpComp_Angle(&prev->rotX,  &next->rotX,  update->rotX,  interpolate);
	}
	if (flags & LU_HAS_ROTZ) {
		LocalInterpComp_Angle(&prev->rotZ,  &next->rotZ,  update->rotZ,  interpolate);
	}
	if (flags & LU_HAS_YAW) {
		LocalInterpComp_Angle(&prev->yaw,   &next->yaw,   update->yaw,   interpolate);

		if (!interpolate) {
			next->rotY        = next->yaw;
			interp->RotYCount = 0;
		} else {
			/* Body Y rotation lags slightly behind */
			InterpComp_AddRotY(interp, Math_LerpAngle(prev->yaw, next->yaw, 0.33333333f));
			InterpComp_AddRotY(interp, Math_LerpAngle(prev->yaw, next->yaw, 0.66666667f));
			InterpComp_AddRotY(interp, Math_LerpAngle(prev->yaw, next->yaw, 1.00000000f));

			e->next.rotY = interp->RotYStates[0];
		}
	}
	Entity_LerpAngles(e, 0.0f);
}

void LocalInterpComp_AdvanceState(struct InterpComp* interp, struct Entity* e) {
	e->prev     = e->next;
	e->Position = e->prev.pos;
	InterpComp_AdvanceRotY(interp, e);
}


/*########################################################################################################################*
*-----------------------------------------------------ShadowComponent-----------------------------------------------------*
*#########################################################################################################################*/
cc_bool ShadowComponent_BoundShadowTex;
GfxResourceID ShadowComponent_ShadowTex;
static float shadow_radius, shadow_uvScale;
struct ShadowData { float Y; BlockID Block; cc_uint8 A; };

static cc_bool lequal(float a, float b) { return a < b || Math_AbsF(a - b) < 0.001f; }
static void ShadowComponent_DrawCoords(struct VertexTextured** vertices, struct Entity* e, struct ShadowData* data, float x1, float z1, float x2, float z2) {
	PackedCol col;
	struct VertexTextured* v;
	Vec3 cen;
	float u1, v1, u2, v2;

	if (lequal(x2, x1) || lequal(z2, z1)) return;
	cen = e->Position;

	u1 = (x1 - cen.X) * shadow_uvScale + 0.5f;
	v1 = (z1 - cen.Z) * shadow_uvScale + 0.5f;
	u2 = (x2 - cen.X) * shadow_uvScale + 0.5f;
	v2 = (z2 - cen.Z) * shadow_uvScale + 0.5f;
	if (u2 <= 0.0f || v2 <= 0.0f || u1 >= 1.0f || v1 >= 1.0f) return;

	x1 = max(x1, cen.X - shadow_radius); u1 = u1 >= 0.0f ? u1 : 0.0f;
	z1 = max(z1, cen.Z - shadow_radius); v1 = v1 >= 0.0f ? v1 : 0.0f;
	x2 = min(x2, cen.X + shadow_radius); u2 = u2 <= 1.0f ? u2 : 1.0f;
	z2 = min(z2, cen.Z + shadow_radius); v2 = v2 <= 1.0f ? v2 : 1.0f;

	v   = *vertices;
	col = PackedCol_Make(255, 255, 255, data->A);

	v->X = x1; v->Y = data->Y; v->Z = z1; v->Col = col; v->U = u1; v->V = v1; v++;
	v->X = x2; v->Y = data->Y; v->Z = z1; v->Col = col; v->U = u2; v->V = v1; v++;
	v->X = x2; v->Y = data->Y; v->Z = z2; v->Col = col; v->U = u2; v->V = v2; v++;
	v->X = x1; v->Y = data->Y; v->Z = z2; v->Col = col; v->U = u1; v->V = v2; v++;

	*vertices = v;
}

static void ShadowComponent_DrawSquareShadow(struct VertexTextured** vertices, float y, float x, float z) {
	PackedCol col = PackedCol_Make(255, 255, 255, 220);
	float     uv1 = 63/128.0f, uv2 = 64/128.0f;
	struct VertexTextured* v = *vertices;

	v->X = x;     v->Y = y; v->Z = z;     v->Col = col; v->U = uv1; v->V = uv1; v++;
	v->X = x + 1; v->Y = y; v->Z = z;     v->Col = col; v->U = uv2; v->V = uv1; v++;
	v->X = x + 1; v->Y = y; v->Z = z + 1; v->Col = col; v->U = uv2; v->V = uv2; v++;
	v->X = x;     v->Y = y; v->Z = z + 1; v->Col = col; v->U = uv1; v->V = uv2; v++;

	*vertices = v;
}

/* Shadow may extend down multiple blocks vertically */
/* If so, shadow on a block must be 'chopped up' to avoid a shadow underneath block above this one */
static void ShadowComponent_DrawCircle(struct VertexTextured** vertices, struct Entity* e, struct ShadowData* data, float x, float z) {
	Vec3 min, max, nMin, nMax;
	int i;
	x = (float)Math_Floor(x); z = (float)Math_Floor(z);
	min = Blocks.MinBB[data[0].Block]; max = Blocks.MaxBB[data[0].Block];

	ShadowComponent_DrawCoords(vertices, e, &data[0], x + min.X, z + min.Z, x + max.X, z + max.Z);
	for (i = 1; i < 4; i++) {
		if (data[i].Block == BLOCK_AIR) return;
		nMin = Blocks.MinBB[data[i].Block]; nMax = Blocks.MaxBB[data[i].Block];

		ShadowComponent_DrawCoords(vertices, e, &data[i], x +  min.X, z + nMin.Z, x +  max.X, z +  min.Z);
		ShadowComponent_DrawCoords(vertices, e, &data[i], x +  min.X, z +  max.Z, x +  max.X, z + nMax.Z);

		ShadowComponent_DrawCoords(vertices, e, &data[i], x + nMin.X, z + nMin.Z, x +  min.X, z + nMax.Z);
		ShadowComponent_DrawCoords(vertices, e, &data[i], x +  max.X, z + nMin.Z, x + nMax.X, z + nMax.Z);
		min = nMin; max = nMax;
	}
}

static void ShadowComponent_CalcAlpha(float playerY, struct ShadowData* data) {
	float height = playerY - data->Y;
	if (height <= 6.0f) {
		data->A = (cc_uint8)(160 - 160 * height / 6.0f);
		data->Y += 1.0f / 64.0f; return;
	}

	data->A = 0;
	if (height <= 16.0f)      data->Y += 1.0f / 64.0f;
	else if (height <= 32.0f) data->Y += 1.0f / 16.0f;
	else if (height <= 96.0f) data->Y += 1.0f / 8.0f;
	else data->Y += 1.0f / 4.0f;
}

static cc_bool ShadowComponent_GetBlocks(struct Entity* e, int x, int y, int z, struct ShadowData* data) {
	struct ShadowData zeroData = { 0 };
	struct ShadowData* cur;
	float posY, topY;
	cc_bool outside;
	BlockID block; cc_uint8 draw;
	int i;

	for (i = 0; i < 4; i++) { data[i] = zeroData; }
	cur     = data;
	posY    = e->Position.Y;
	outside = !World_ContainsXZ(x, z);

	for (i = 0; y >= 0 && i < 4; y--) {
		if (!outside) {
			block = World_GetBlock(x, y, z);
		} else if (y == Env.EdgeHeight - 1) {
			block = Blocks.Draw[Env.EdgeBlock] == DRAW_GAS  ? BLOCK_AIR : BLOCK_BEDROCK;
		} else if (y == Env_SidesHeight - 1) {
			block = Blocks.Draw[Env.SidesBlock] == DRAW_GAS ? BLOCK_AIR : BLOCK_BEDROCK;
		} else {
			block = BLOCK_AIR;
		}

		draw = Blocks.Draw[block];
		if (draw == DRAW_GAS || draw == DRAW_SPRITE || Blocks.IsLiquid[block]) continue;
		topY = y + Blocks.MaxBB[block].Y;
		if (topY >= posY + 0.01f) continue;

		cur->Block = block; cur->Y = topY;
		ShadowComponent_CalcAlpha(posY, cur);
		i++; cur++;

		/* Check if the casted shadow will continue on further down. */
		if (Blocks.MinBB[block].X == 0.0f && Blocks.MaxBB[block].X == 1.0f &&
			Blocks.MinBB[block].Z == 0.0f && Blocks.MaxBB[block].Z == 1.0f) return true;
	}

	if (i < 4) {
		cur->Block = Env.EdgeBlock; cur->Y = 0.0f;
		ShadowComponent_CalcAlpha(posY, cur);
		i++; cur++;
	}
	return true;
}

#define sh_size 128
#define sh_half (sh_size / 2)
static void ShadowComponent_MakeTex(void) {
	BitmapCol pixels[sh_size * sh_size];
	BitmapCol color = BitmapCol_Make(0, 0, 0, 200);
	struct Bitmap bmp;
	cc_uint32 x, y;

	Bitmap_Init(bmp, sh_size, sh_size, pixels);
	for (y = 0; y < sh_size; y++) {
		BitmapCol* row = Bitmap_GetRow(&bmp, y);

		for (x = 0; x < sh_size; x++) {
			double dist = 
				(sh_half - (x + 0.5)) * (sh_half - (x + 0.5)) +
				(sh_half - (y + 0.5)) * (sh_half - (y + 0.5));
			row[x] = dist < sh_half * sh_half ? color : 0;
		}
	}
	Gfx_RecreateTexture(&ShadowComponent_ShadowTex, &bmp, 0, false);
}

void ShadowComponent_Draw(struct Entity* e) {
	struct VertexTextured vertices[128];
	struct VertexTextured* ptr;
	struct ShadowData data[4];
	GfxResourceID vb;
	Vec3 pos;
	float radius;
	int y, count;
	int x1, z1, x2, z2;

	pos = e->Position;
	if (pos.Y < 0.0f) return;
	y = min((int)pos.Y, World.MaxY);

	radius = 7.0f * min(e->ModelScale.Y, 1.0f) * e->Model->shadowScale;
	shadow_radius  = radius / 16.0f;
	shadow_uvScale = 16.0f / (radius * 2.0f);

	/* TODO: Should shadow component use its own VB? */
	ptr = vertices;
	if (Entities.ShadowsMode == SHADOW_MODE_SNAP_TO_BLOCK) {
		vb = Gfx_texVb;
		x1 = Math_Floor(pos.X); z1 = Math_Floor(pos.Z);
		if (!ShadowComponent_GetBlocks(e, x1, y, z1, data)) return;

		ShadowComponent_DrawSquareShadow(&ptr, data[0].Y, x1, z1);
	} else {
		vb = Models.Vb;
		x1 = Math_Floor(pos.X - shadow_radius); z1 = Math_Floor(pos.Z - shadow_radius);
		x2 = Math_Floor(pos.X + shadow_radius); z2 = Math_Floor(pos.Z + shadow_radius);

		if (ShadowComponent_GetBlocks(e, x1, y, z1, data) && data[0].A > 0) {
			ShadowComponent_DrawCircle(&ptr, e, data, (float)x1, (float)z1);
		}
		if (x1 != x2 && ShadowComponent_GetBlocks(e, x2, y, z1, data) && data[0].A > 0) {
			ShadowComponent_DrawCircle(&ptr, e, data, (float)x2, (float)z1);
		}
		if (z1 != z2 && ShadowComponent_GetBlocks(e, x1, y, z2, data) && data[0].A > 0) {
			ShadowComponent_DrawCircle(&ptr, e, data, (float)x1, (float)z2);
		}
		if (x1 != x2 && z1 != z2 && ShadowComponent_GetBlocks(e, x2, y, z2, data) && data[0].A > 0) {
			ShadowComponent_DrawCircle(&ptr, e, data, (float)x2, (float)z2);
		}
	}

	if (ptr == vertices) return;
	if (!ShadowComponent_ShadowTex) ShadowComponent_MakeTex();

	if (!ShadowComponent_BoundShadowTex) {
		Gfx_BindTexture(ShadowComponent_ShadowTex);
		ShadowComponent_BoundShadowTex = true;
	}

	count = (int)(ptr - vertices);
	Gfx_UpdateDynamicVb_IndexedTris(vb, vertices, count);
}


/*########################################################################################################################*
*---------------------------------------------------CollisionsComponent---------------------------------------------------*
*#########################################################################################################################*/
/* Whether a collision occurred with any horizontal sides of any blocks */
cc_bool Collisions_HitHorizontal(struct CollisionsComp* comp) {
	return comp->HitXMin || comp->HitXMax || comp->HitZMin || comp->HitZMax;
}
#define COLLISIONS_ADJ 0.001f

static void Collisions_ClipX(struct Entity* e, Vec3* size, struct AABB* entityBB, struct AABB* extentBB) {
	e->Velocity.X = 0.0f;
	entityBB->Min.X = e->Position.X - size->X / 2; extentBB->Min.X = entityBB->Min.X;
	entityBB->Max.X = e->Position.X + size->X / 2; extentBB->Max.X = entityBB->Max.X;
}

static void Collisions_ClipY(struct Entity* e, Vec3* size, struct AABB* entityBB, struct AABB* extentBB) {
	e->Velocity.Y = 0.0f;
	entityBB->Min.Y = e->Position.Y;               extentBB->Min.Y = entityBB->Min.Y;
	entityBB->Max.Y = e->Position.Y + size->Y;     extentBB->Max.Y = entityBB->Max.Y;
}

static void Collisions_ClipZ(struct Entity* e, Vec3* size, struct AABB* entityBB, struct AABB* extentBB) {
	e->Velocity.Z = 0.0f;
	entityBB->Min.Z = e->Position.Z - size->Z / 2; extentBB->Min.Z = entityBB->Min.Z;
	entityBB->Max.Z = e->Position.Z + size->Z / 2; extentBB->Max.Z = entityBB->Max.Z;
}

static cc_bool Collisions_CanSlideThrough(struct AABB* adjFinalBB) {
	IVec3 bbMin, bbMax; 
	struct AABB blockBB;
	BlockID block;
	Vec3 v;
	int x, y, z;

	IVec3_Floor(&bbMin, &adjFinalBB->Min);
	IVec3_Floor(&bbMax, &adjFinalBB->Max);

	for (y = bbMin.Y; y <= bbMax.Y; y++) { v.Y = (float)y;
		for (z = bbMin.Z; z <= bbMax.Z; z++) { v.Z = (float)z;
			for (x = bbMin.X; x <= bbMax.X; x++) { v.X = (float)x;

				block = World_GetPhysicsBlock(x, y, z);
				Vec3_Add(&blockBB.Min, &v, &Blocks.MinBB[block]);
				Vec3_Add(&blockBB.Max, &v, &Blocks.MaxBB[block]);

				if (!AABB_Intersects(&blockBB, adjFinalBB)) continue;
				if (Blocks.Collide[block] == COLLIDE_SOLID) return false;
			}
		}
	}
	return true;
}

static cc_bool Collisions_DidSlide(struct CollisionsComp* comp, struct AABB* blockBB, Vec3* size,
									struct AABB* finalBB, struct AABB* entityBB, struct AABB* extentBB) {
	float yDist = blockBB->Max.Y - entityBB->Min.Y;
	struct AABB adjBB;

	if (yDist > 0.0f && yDist <= comp->StepSize + 0.01f) {
		float blockBB_MinX = max(blockBB->Min.X, blockBB->Max.X - size->X / 2);
		float blockBB_MaxX = min(blockBB->Max.X, blockBB->Min.X + size->X / 2);
		float blockBB_MinZ = max(blockBB->Min.Z, blockBB->Max.Z - size->Z / 2);
		float blockBB_MaxZ = min(blockBB->Max.Z, blockBB->Min.Z + size->Z / 2);
		
		adjBB.Min.X = min(finalBB->Min.X, blockBB_MinX + COLLISIONS_ADJ);
		adjBB.Max.X = max(finalBB->Max.X, blockBB_MaxX - COLLISIONS_ADJ);
		adjBB.Min.Y = blockBB->Max.Y + COLLISIONS_ADJ;
		adjBB.Max.Y = adjBB.Min.Y    + size->Y;
		adjBB.Min.Z = min(finalBB->Min.Z, blockBB_MinZ + COLLISIONS_ADJ);
		adjBB.Max.Z = max(finalBB->Max.Z, blockBB_MaxZ - COLLISIONS_ADJ);

		if (!Collisions_CanSlideThrough(&adjBB)) return false;
		comp->Entity->Position.Y = adjBB.Min.Y;
		comp->Entity->OnGround = true;
		Collisions_ClipY(comp->Entity, size, entityBB, extentBB);
		return true;
	}
	return false;
}

static void Collisions_ClipXMin(struct CollisionsComp* comp, struct AABB* blockBB, struct AABB* entityBB,
								cc_bool wasOn, struct AABB* finalBB, struct AABB* extentBB, Vec3* size) {
	if (!wasOn || !Collisions_DidSlide(comp, blockBB, size, finalBB, entityBB, extentBB)) {
		comp->Entity->Position.X = blockBB->Min.X - size->X / 2 - COLLISIONS_ADJ;
		Collisions_ClipX(comp->Entity, size, entityBB, extentBB);
		comp->HitXMin = true;
	}
}

static void Collisions_ClipXMax(struct CollisionsComp* comp, struct AABB* blockBB, struct AABB* entityBB, 
								cc_bool wasOn, struct AABB* finalBB, struct AABB* extentBB, Vec3* size) {
	if (!wasOn || !Collisions_DidSlide(comp, blockBB, size, finalBB, entityBB, extentBB)) {
		comp->Entity->Position.X = blockBB->Max.X + size->X / 2 + COLLISIONS_ADJ;
		Collisions_ClipX(comp->Entity, size, entityBB, extentBB);
		comp->HitXMax = true;
	}
}

static void Collisions_ClipZMax(struct CollisionsComp* comp, struct AABB* blockBB, struct AABB* entityBB, 
								cc_bool wasOn, struct AABB* finalBB, struct AABB* extentBB, Vec3* size) {
	if (!wasOn || !Collisions_DidSlide(comp, blockBB, size, finalBB, entityBB, extentBB)) {
		comp->Entity->Position.Z = blockBB->Max.Z + size->Z / 2 + COLLISIONS_ADJ;
		Collisions_ClipZ(comp->Entity, size, entityBB, extentBB);
		comp->HitZMax = true;
	}
}

static void Collisions_ClipZMin(struct CollisionsComp* comp, struct AABB* blockBB, struct AABB* entityBB,
								cc_bool wasOn, struct AABB* finalBB, struct AABB* extentBB, Vec3* size) {
	if (!wasOn || !Collisions_DidSlide(comp, blockBB, size, finalBB, entityBB, extentBB)) {
		comp->Entity->Position.Z = blockBB->Min.Z - size->Z / 2 - COLLISIONS_ADJ;
		Collisions_ClipZ(comp->Entity, size, entityBB, extentBB);
		comp->HitZMin = true;
	}
}

static void Collisions_ClipYMin(struct CollisionsComp* comp, struct AABB* blockBB, struct AABB* entityBB, 
								struct AABB* extentBB, Vec3* size) {
	comp->Entity->Position.Y = blockBB->Min.Y - size->Y - COLLISIONS_ADJ;
	Collisions_ClipY(comp->Entity, size, entityBB, extentBB);
	comp->HitYMin = true;
}

static void Collisions_ClipYMax(struct CollisionsComp* comp, struct AABB* blockBB, struct AABB* entityBB, 
								struct AABB* extentBB, Vec3* size) {
	comp->Entity->Position.Y = blockBB->Max.Y + COLLISIONS_ADJ;
	comp->Entity->OnGround = true;
	Collisions_ClipY(comp->Entity, size, entityBB, extentBB);
	comp->HitYMax = true;
}

static void Collisions_CollideWithReachableBlocks(struct CollisionsComp* comp, int count, struct AABB* entityBB,
												struct AABB* extentBB) {
	struct Entity* entity = comp->Entity;
	struct SearcherState state;
	struct AABB blockBB, finalBB;
	Vec3 size;
	cc_bool wasOn;

	Vec3 bPos, v;
	float tx, ty, tz;
	int i, block;

	/* Reset collision detection states */
	wasOn = entity->OnGround;
	entity->OnGround = false;
	comp->HitXMin = false; comp->HitYMin = false; comp->HitZMin = false;
	comp->HitXMax = false; comp->HitYMax = false; comp->HitZMax = false;

	size = entity->Size;
	for (i = 0; i < count; i++) {
		/* Unpack the block and coordinate data */
		state  = Searcher_States[i];
		bPos.X = state.X >> 3; bPos.Y = state.Y >> 4; bPos.Z = state.Z >> 3;
		block  = (state.X & 0x7) | (state.Y & 0xF) << 3 | (state.Z & 0x7) << 7;

		Vec3_Add(&blockBB.Min, &Blocks.MinBB[block], &bPos);
		Vec3_Add(&blockBB.Max, &Blocks.MaxBB[block], &bPos);
		if (!AABB_Intersects(extentBB, &blockBB)) continue;

		/* Recheck time to collide with block (as colliding with blocks modifies this) */
		Searcher_CalcTime(&entity->Velocity, entityBB, &blockBB, &tx, &ty, &tz);
		if (tx > 1.0f || ty > 1.0f || tz > 1.0f) {
			Platform_LogConst("t > 1 in physics calculation.. this shouldn't have happened.");
		}

		/* Calculate the location of the entity when it collides with this block */
		v = entity->Velocity; 
		v.X *= tx; v.Y *= ty; v.Z *= tz;
		/* Inlined ABBB_Offset */
		Vec3_Add(&finalBB.Min, &entityBB->Min, &v);
		Vec3_Add(&finalBB.Max, &entityBB->Max, &v);

		/* if we have hit the bottom of a block, we need to change the axis we test first */
		if (!comp->HitYMin) {
			if (finalBB.Min.Y + COLLISIONS_ADJ >= blockBB.Max.Y) {
				Collisions_ClipYMax(comp, &blockBB, entityBB, extentBB, &size);
			} else if (finalBB.Max.Y - COLLISIONS_ADJ <= blockBB.Min.Y) {
				Collisions_ClipYMin(comp, &blockBB, entityBB, extentBB, &size);
			} else if (finalBB.Min.X + COLLISIONS_ADJ >= blockBB.Max.X) {
				Collisions_ClipXMax(comp, &blockBB, entityBB, wasOn, &finalBB, extentBB, &size);
			} else if (finalBB.Max.X - COLLISIONS_ADJ <= blockBB.Min.X) {
				Collisions_ClipXMin(comp, &blockBB, entityBB, wasOn, &finalBB, extentBB, &size);
			} else if (finalBB.Min.Z + COLLISIONS_ADJ >= blockBB.Max.Z) {
				Collisions_ClipZMax(comp, &blockBB, entityBB, wasOn, &finalBB, extentBB, &size);
			} else if (finalBB.Max.Z - COLLISIONS_ADJ <= blockBB.Min.Z) {
				Collisions_ClipZMin(comp, &blockBB, entityBB, wasOn, &finalBB, extentBB, &size);
			}
			continue;
		}

		/* if flying or falling, test the horizontal axes first */
		if (finalBB.Min.X + COLLISIONS_ADJ >= blockBB.Max.X) {
			Collisions_ClipXMax(comp, &blockBB, entityBB, wasOn, &finalBB, extentBB, &size);
		} else if (finalBB.Max.X - COLLISIONS_ADJ <= blockBB.Min.X) {
			Collisions_ClipXMin(comp, &blockBB, entityBB, wasOn, &finalBB, extentBB, &size);
		} else if (finalBB.Min.Z + COLLISIONS_ADJ >= blockBB.Max.Z) {
			Collisions_ClipZMax(comp, &blockBB, entityBB, wasOn, &finalBB, extentBB, &size);
		} else if (finalBB.Max.Z - COLLISIONS_ADJ <= blockBB.Min.Z) {
			Collisions_ClipZMin(comp, &blockBB, entityBB, wasOn, &finalBB, extentBB, &size);
		} else if (finalBB.Min.Y + COLLISIONS_ADJ >= blockBB.Max.Y) {
			Collisions_ClipYMax(comp, &blockBB, entityBB, extentBB, &size);
		} else if (finalBB.Max.Y - COLLISIONS_ADJ <= blockBB.Min.Y) {
			Collisions_ClipYMin(comp, &blockBB, entityBB, extentBB, &size);
		}
	}
}

/* TODO: test for corner cases, and refactor this */
void Collisions_MoveAndWallSlide(struct CollisionsComp* comp) {
	struct Entity* e = comp->Entity;
	struct AABB entityBB, entityExtentBB;
	int count;

	if (Vec3_IsZero(e->Velocity)) return;
	count = Searcher_FindReachableBlocks(e,            &entityBB, &entityExtentBB);
	Collisions_CollideWithReachableBlocks(comp, count, &entityBB, &entityExtentBB);
}


/*########################################################################################################################*
*----------------------------------------------------PhysicsComponent-----------------------------------------------------*
*#########################################################################################################################*/
void PhysicsComp_Init(struct PhysicsComp* comp, struct Entity* entity) {
	Mem_Set(comp, 0, sizeof(struct PhysicsComp));
	comp->CanLiquidJump = true;
	comp->Entity = entity;
	comp->JumpVel       = 0.42f;
	comp->UserJumpVel   = 0.42f;
	comp->ServerJumpVel = 0.42f;
}

static cc_bool PhysicsComp_TouchesLiquid(BlockID block) { return Blocks.Collide[block] == COLLIDE_LIQUID; }
void PhysicsComp_UpdateVelocityState(struct PhysicsComp* comp) {
	struct Entity* entity   = comp->Entity;
	struct HacksComp* hacks = comp->Hacks;
	struct AABB bounds;
	int dir;

	cc_bool touchWater, touchLava;
	cc_bool liquidFeet, liquidRest;
	int feetY, bodyY, headY;
	cc_bool pastJumpPoint;

	if (hacks->Floating) {
		entity->Velocity.Y = 0.0f; /* eliminate the effect of gravity */
		dir = (hacks->FlyingUp || comp->Jumping) ? 1 : (hacks->FlyingDown ? -1 : 0);

		entity->Velocity.Y += 0.12f * dir;
		if (hacks->Speeding     && hacks->CanSpeed) entity->Velocity.Y += 0.12f * dir;
		if (hacks->HalfSpeeding && hacks->CanSpeed) entity->Velocity.Y += 0.06f * dir;
	} else if (comp->Jumping && Entity_TouchesAnyRope(entity) && entity->Velocity.Y > 0.02f) {
		entity->Velocity.Y = 0.02f;
	}

	if (!comp->Jumping) { comp->CanLiquidJump = false; return; }
	touchWater = Entity_TouchesAnyWater(entity);
	touchLava  = Entity_TouchesAnyLava(entity);

	if (touchWater || touchLava) {
		Entity_GetBounds(entity, &bounds);
		feetY = Math_Floor(bounds.Min.Y); bodyY = feetY + 1;
		headY = Math_Floor(bounds.Max.Y);
		if (bodyY > headY) bodyY = headY;

		bounds.Max.Y = bounds.Min.Y = feetY;
		liquidFeet   = Entity_TouchesAny(&bounds, PhysicsComp_TouchesLiquid);
		bounds.Min.Y = min(bodyY, headY);
		bounds.Max.Y = max(bodyY, headY);
		liquidRest   = Entity_TouchesAny(&bounds, PhysicsComp_TouchesLiquid);

		pastJumpPoint = liquidFeet && !liquidRest && (Math_Mod1(entity->Position.Y) >= 0.4f);
		if (!pastJumpPoint) {
			comp->CanLiquidJump = true;
			entity->Velocity.Y += 0.04f;
			if (hacks->Speeding     && hacks->CanSpeed) entity->Velocity.Y += 0.04f;
			if (hacks->HalfSpeeding && hacks->CanSpeed) entity->Velocity.Y += 0.02f;
		} else if (pastJumpPoint) {
			/* either A) climb up solid on side B) jump bob in water */
			if (Collisions_HitHorizontal(comp->Collisions)) {
				entity->Velocity.Y += touchLava ? 0.30f : 0.13f;
			} else if (comp->CanLiquidJump) {
				entity->Velocity.Y += touchLava ? 0.20f : 0.10f;
			}
			comp->CanLiquidJump = false;
		}
	} else if (comp->UseLiquidGravity) {
		entity->Velocity.Y += 0.04f;
		if (hacks->Speeding     && hacks->CanSpeed) entity->Velocity.Y += 0.04f;
		if (hacks->HalfSpeeding && hacks->CanSpeed) entity->Velocity.Y += 0.02f;
		comp->CanLiquidJump = false;
	} else if (Entity_TouchesAnyRope(entity)) {
		entity->Velocity.Y += (hacks->Speeding && hacks->CanSpeed) ? 0.15f : 0.10f;
		comp->CanLiquidJump = false;
	} else if (entity->OnGround) {
		PhysicsComp_DoNormalJump(comp);
	}
}

void PhysicsComp_DoNormalJump(struct PhysicsComp* comp) {
	struct Entity* entity   = comp->Entity;
	struct HacksComp* hacks = comp->Hacks;
	if (comp->JumpVel == 0.0f || hacks->MaxJumps <= 0) return;

	entity->Velocity.Y = comp->JumpVel;
	if (hacks->Speeding     && hacks->CanSpeed) entity->Velocity.Y += comp->JumpVel;
	if (hacks->HalfSpeeding && hacks->CanSpeed) entity->Velocity.Y += comp->JumpVel / 2;
	comp->CanLiquidJump = false;
}

static cc_bool PhysicsComp_TouchesSlipperyIce(BlockID b) { return Blocks.ExtendedCollide[b] == COLLIDE_SLIPPERY_ICE; }
static cc_bool PhysicsComp_OnIce(struct Entity* e) {
	struct AABB bounds;
	int feetX, feetY, feetZ;
	BlockID feetBlock;

	feetX = Math_Floor(e->Position.X);
	feetY = Math_Floor(e->Position.Y - 0.01f);
	feetZ = Math_Floor(e->Position.Z);

	feetBlock = World_GetPhysicsBlock(feetX, feetY, feetZ);
	if (Blocks.ExtendedCollide[feetBlock] == COLLIDE_ICE) return true;

	Entity_GetBounds(e, &bounds);
	bounds.Min.Y -= 0.01f; bounds.Max.Y = bounds.Min.Y;
	return Entity_TouchesAny(&bounds, PhysicsComp_TouchesSlipperyIce);
}

static void PhysicsComp_MoveHor(struct PhysicsComp* comp, Vec3 vel, float factor) {
	struct Entity* entity;
	float dist;

	dist = Math_SqrtF(vel.X * vel.X + vel.Z * vel.Z);
	if (dist < 0.00001f) return;
	if (dist < 1.0f) dist = 1.0f;

	/* entity.Velocity += vel * (factor / dist) */
	entity = comp->Entity;
	Vec3_Mul1By(&vel, factor / dist);
	Vec3_AddBy(&entity->Velocity, &vel);
}

static void PhysicsComp_Move(struct PhysicsComp* comp, Vec3 drag, float gravity, float yMul) {
	struct Entity* entity = comp->Entity;
	entity->Velocity.Y *= yMul;

	if (!comp->Hacks->Noclip) {
		Collisions_MoveAndWallSlide(comp->Collisions);
	}
	Vec3_AddBy(&entity->Position, &entity->Velocity);

	entity->Velocity.Y /= yMul;
	Vec3_Mul3By(&entity->Velocity, &drag);
	entity->Velocity.Y -= gravity;
}

static void PhysicsComp_MoveFlying(struct PhysicsComp* comp, Vec3 vel, float factor, Vec3 drag, float gravity, float yMul) {
	struct Entity* entity   = comp->Entity;
	struct HacksComp* hacks = comp->Hacks;
	float yVel;

	PhysicsComp_MoveHor(comp, vel, factor);
	yVel = Math_SqrtF(entity->Velocity.X * entity->Velocity.X + entity->Velocity.Z * entity->Velocity.Z);
	/* make horizontal speed the same as vertical speed */
	if ((vel.X != 0.0f || vel.Z != 0.0f) && yVel > 0.001f) {
		entity->Velocity.Y = 0.0f;
		yMul = 1.0f;
		if (hacks->FlyingUp || comp->Jumping) entity->Velocity.Y += yVel;
		if (hacks->FlyingDown)                entity->Velocity.Y -= yVel;
	}
	PhysicsComp_Move(comp, drag, gravity, yMul);
}

static void PhysicsComp_MoveNormal(struct PhysicsComp* comp, Vec3 vel, float factor, Vec3 drag, float gravity, float yMul) {
	PhysicsComp_MoveHor(comp, vel, factor);
	PhysicsComp_Move(comp, drag, gravity, yMul);
}

static float PhysicsComp_LowestModifier(struct PhysicsComp* comp, struct AABB* bounds, cc_bool checkSolid) {
	IVec3 bbMin, bbMax;
	float modifier = MATH_LARGENUM;
	struct AABB blockBB;
	BlockID block;
	cc_uint8 collide;
	Vec3 v;
	int x, y, z;

	IVec3_Floor(&bbMin, &bounds->Min);
	IVec3_Floor(&bbMax, &bounds->Max);	

	bbMin.X = max(bbMin.X, 0); bbMax.X = min(bbMax.X, World.MaxX);
	bbMin.Y = max(bbMin.Y, 0); bbMax.Y = min(bbMax.Y, World.MaxY);
	bbMin.Z = max(bbMin.Z, 0); bbMax.Z = min(bbMax.Z, World.MaxZ);
	
	for (y = bbMin.Y; y <= bbMax.Y; y++) { v.Y = (float)y;
		for (z = bbMin.Z; z <= bbMax.Z; z++) { v.Z = (float)z;
			for (x = bbMin.X; x <= bbMax.X; x++) { v.X = (float)x;
				block = World_GetBlock(x, y, z);

				if (block == BLOCK_AIR) continue;
				collide = Blocks.Collide[block];
				if (collide == COLLIDE_SOLID && !checkSolid) continue;

				Vec3_Add(&blockBB.Min, &v, &Blocks.MinBB[block]);
				Vec3_Add(&blockBB.Max, &v, &Blocks.MaxBB[block]);
				if (!AABB_Intersects(&blockBB, bounds)) continue;

				modifier = min(modifier, Blocks.SpeedMultiplier[block]);
				if (Blocks.ExtendedCollide[block] == COLLIDE_LIQUID) {
					comp->UseLiquidGravity = true;
				}
			}
		}
	}
	return modifier;
}

static float PhysicsComp_GetSpeed(struct HacksComp* hacks, float speedMul, cc_bool canSpeed) {
	float factor = hacks->Floating ? speedMul : 1.0f;
	float speed  = factor * (1 + HacksComp_CalcSpeedFactor(hacks, canSpeed));
	return hacks->CanSpeed ? speed : min(speed, hacks->MaxHorSpeed);
}

static float PhysicsComp_GetBaseSpeed(struct PhysicsComp* comp) {
	struct AABB bounds;
	float baseModifier, solidModifier;
	
	Entity_GetBounds(comp->Entity, &bounds);
	comp->UseLiquidGravity = false;

	baseModifier  = PhysicsComp_LowestModifier(comp, &bounds, false);
	bounds.Min.Y -= 0.5f/16.0f; /* also check block standing on */
	solidModifier = PhysicsComp_LowestModifier(comp, &bounds, true);

	if (baseModifier == MATH_LARGENUM && solidModifier == MATH_LARGENUM) return 1.0f;
	return baseModifier == MATH_LARGENUM ? solidModifier : baseModifier;
}

#define LIQUID_GRAVITY 0.02f
#define ROPE_GRAVITY   0.034f
void PhysicsComp_PhysicsTick(struct PhysicsComp* comp, Vec3 vel) {
	struct Entity* entity   = comp->Entity;
	struct HacksComp* hacks = comp->Hacks;
	float baseSpeed, verSpeed, horSpeed;
	float factor, gravity;
	cc_bool womSpeedBoost;

	if (hacks->Noclip) entity->OnGround = false;
	baseSpeed = PhysicsComp_GetBaseSpeed(comp);
	verSpeed  = baseSpeed * (PhysicsComp_GetSpeed(hacks, 8.0f, hacks->CanSpeed) / 5.0f);
	horSpeed  = baseSpeed * PhysicsComp_GetSpeed(hacks,  8.0f / 5.0f, true) * hacks->BaseHorSpeed;
	/* previously horSpeed used to be multiplied by factor of 0.02 in last case */
	/* it's now multiplied by 0.1, so need to divide by 5 so user speed modifier comes out same */

	/* TODO: this is a temp fix to avoid crashing for high horizontal speed */
	Math_Clamp(horSpeed, -75.0f, 75.0f);
	/* vertical speed never goes below: base speed * 1.0 */
	if (verSpeed < baseSpeed) verSpeed = baseSpeed;

	womSpeedBoost = hacks->CanDoubleJump && hacks->WOMStyleHacks;
	if (!hacks->Floating && womSpeedBoost) {
		if (comp->MultiJumps == 1)     { horSpeed *= 46.5f; verSpeed *= 7.5f; } 
		else if (comp->MultiJumps > 1) { horSpeed *= 93.0f; verSpeed *= 10.0f; }
	}

	if (Entity_TouchesAnyWater(entity) && !hacks->Floating) {
		Vec3 waterDrag = { 0.8f, 0.8f, 0.8f };
		PhysicsComp_MoveNormal(comp, vel, 0.02f * horSpeed, waterDrag, LIQUID_GRAVITY, verSpeed);
	} else if (Entity_TouchesAnyLava(entity) && !hacks->Floating) {
		Vec3 lavaDrag = { 0.5f, 0.5f, 0.5f };
		PhysicsComp_MoveNormal(comp, vel, 0.02f * horSpeed, lavaDrag, LIQUID_GRAVITY, verSpeed);
	} else if (Entity_TouchesAnyRope(entity) && !hacks->Floating) {
		Vec3 ropeDrag = { 0.5f, 0.85f, 0.5f };
		PhysicsComp_MoveNormal(comp, vel, 0.02f * 1.7f, ropeDrag, ROPE_GRAVITY, verSpeed);
	} else {
		factor  = hacks->Floating || entity->OnGround ? 0.1f : 0.02f;
		gravity = comp->UseLiquidGravity ? LIQUID_GRAVITY : entity->Model->gravity;

		if (hacks->Floating) {
			PhysicsComp_MoveFlying(comp, vel, factor * horSpeed, entity->Model->drag, gravity, verSpeed);
		} else {
			PhysicsComp_MoveNormal(comp, vel, factor * horSpeed, entity->Model->drag, gravity, verSpeed);
		}

		if (PhysicsComp_OnIce(entity) && !hacks->Floating) {
			/* limit components to +-0.25f by rescaling vector to [-0.25, 0.25] */
			if (Math_AbsF(entity->Velocity.X) > 0.25f || Math_AbsF(entity->Velocity.Z) > 0.25f) {
				float xScale = Math_AbsF(0.25f / entity->Velocity.X);
				float zScale = Math_AbsF(0.25f / entity->Velocity.Z);

				float scale = min(xScale, zScale);
				entity->Velocity.X *= scale;
				entity->Velocity.Z *= scale;
			}
		} else if (entity->OnGround || hacks->Flying) {
			Vec3_Mul3By(&entity->Velocity, &entity->Model->groundFriction); /* air drag or ground friction */
		}
	}

	if (entity->OnGround) comp->MultiJumps = 0;
}

static double PhysicsComp_YPosAt(int t, float u) {
	/* v(t, u) = (4 + u) * (0.98^t) - 4, where u = initial velocity */
	/* x(t, u) = Σv(t, u) from 0 to t (since we work in discrete timesteps) */
	/* plugging into Wolfram Alpha gives 1 equation as */
	/* (0.98^t) * (-49u - 196) - 4t + 50u + 196 */
	double a = Math_Exp(-0.0202027 * t); /* ~0.98^t */
	return a * (-49 * u - 196) - 4 * t + 50 * u + 196;
}

double PhysicsComp_CalcMaxHeight(float u) {
	/* equation below comes from solving diff(x(t, u))= 0 */
	/* We only work in discrete timesteps, so test both rounded up and down */
	double t = 49.49831645 * Math_Log(0.247483075 * u + 0.9899323);
	double value_floor = PhysicsComp_YPosAt((int)t,     u);
	double value_ceil  = PhysicsComp_YPosAt((int)t + 1, u);
	return max(value_floor, value_ceil);
}

/* Calculates the jump velocity required such that when user presses
the jump binding they will be able to jump up to the given height. */
float PhysicsComp_CalcJumpVelocity(float jumpHeight) {
	float jumpVel = 0.0f;
	if (jumpHeight == 0.0f) return jumpVel;

	if (jumpHeight >= 256.0f) jumpVel = 10.0f;
	if (jumpHeight >= 512.0f) jumpVel = 16.5f;
	if (jumpHeight >= 768.0f) jumpVel = 22.5f;

	while (PhysicsComp_CalcMaxHeight(jumpVel) <= jumpHeight) { jumpVel += 0.001f; }
	return jumpVel;
}

void PhysicsComp_DoEntityPush(struct Entity* entity) {
	struct Entity* other;
	cc_bool yIntersects;
	Vec3 dir;
	float dist, pushStrength;
	int id;
	dir.Y = 0.0f;

	for (id = 0; id < ENTITIES_MAX_COUNT; id++) {
		other = Entities.List[id];
		if (!other || other == entity) continue;
		if (!other->Model->pushes)     continue;

		yIntersects =
			entity->Position.Y <= (other->Position.Y  + other->Size.Y) &&
			 other->Position.Y <= (entity->Position.Y + entity->Size.Y);
		if (!yIntersects) continue;

		dir.X = other->Position.X - entity->Position.X;
		dir.Z = other->Position.Z - entity->Position.Z;
		dist = dir.X * dir.X + dir.Z * dir.Z;
		if (dist < 0.002f || dist > 1.0f) continue; /* TODO: range needs to be lower? */

		Vec3_Normalise(&dir);
		pushStrength = (1 - dist) / 32.0f; /* TODO: should be 24/25 */
		/* entity.Velocity -= dir * pushStrength */
		Vec3_Mul1By(&dir, pushStrength);
		Vec3_SubBy(&entity->Velocity, &dir);
	}
}


/*########################################################################################################################*
*----------------------------------------------------SoundsComponent------------------------------------------------------*
*#########################################################################################################################*/
static Vec3 sounds_lastPos = { -1e25f, -1e25f, -1e25f };
static cc_bool  sounds_anyNonAir;
static cc_uint8 sounds_type;

static cc_bool Sounds_CheckNonSolid(BlockID b) {
	cc_uint8 type = Blocks.StepSounds[b];
	cc_uint8 collide = Blocks.Collide[b];
	if (type != SOUND_NONE && collide != COLLIDE_SOLID) sounds_type = type;

	if (Blocks.Draw[b] != DRAW_GAS) sounds_anyNonAir = true;
	return false;
}

static cc_bool Sounds_CheckSolid(BlockID b) {
	cc_uint8 type = Blocks.StepSounds[b];
	if (type != SOUND_NONE) sounds_type = type;

	if (Blocks.Draw[b] != DRAW_GAS) sounds_anyNonAir = true;
	return false;
}

static void SoundComp_GetSound(struct LocalPlayer* p) {
	struct AABB bounds;
	Vec3 pos;
	IVec3 coords;
	BlockID blockUnder;
	float maxY;
	cc_uint8 typeUnder, collideUnder;

	Entity_GetBounds(&p->Base, &bounds);
	sounds_type = SOUND_NONE;
	sounds_anyNonAir = false;

	/* first check surrounding liquids/gas for sounds */
	Entity_TouchesAny(&bounds, Sounds_CheckNonSolid);
	if (sounds_type != SOUND_NONE) return;

	/* then check block standing on (feet) */
	pos = p->Base.next.pos; pos.Y -= 0.01f;
	IVec3_Floor(&coords, &pos);
	blockUnder = World_SafeGetBlock(coords.X, coords.Y, coords.Z);
	maxY = coords.Y + Blocks.MaxBB[blockUnder].Y;

	typeUnder    = Blocks.StepSounds[blockUnder];
	collideUnder = Blocks.Collide[blockUnder];
	if (maxY >= pos.Y && collideUnder == COLLIDE_SOLID && typeUnder != SOUND_NONE) {
		sounds_anyNonAir = true; sounds_type = typeUnder; return;
	}

	/* then check all solid blocks at feet */
	bounds.Max.Y = bounds.Min.Y = pos.Y;
	Entity_TouchesAny(&bounds, Sounds_CheckSolid);
}

static cc_bool SoundComp_ShouldPlay(struct LocalPlayer* p, Vec3 soundPos) {
	Vec3 delta;
	float distSq;
	float oldLegRot, newLegRot;

	Vec3_Sub(&delta, &sounds_lastPos, &soundPos);
	distSq = Vec3_LengthSquared(&delta);
	/* just play every certain block interval when not animating */
	if (p->Base.Anim.Swing < 0.999f) return distSq > 1.75f * 1.75f;

	/* have our legs just crossed over the '0' point? */
	if (Camera.Active->isThirdPerson) {
		oldLegRot = (float)Math_Cos(p->Base.Anim.WalkTimeO);
		newLegRot = (float)Math_Cos(p->Base.Anim.WalkTimeN);
	} else {
		oldLegRot = (float)Math_Sin(p->Base.Anim.WalkTimeO);
		newLegRot = (float)Math_Sin(p->Base.Anim.WalkTimeN);
	}
	return Math_Sign(oldLegRot) != Math_Sign(newLegRot);
}

void SoundComp_Tick(cc_bool wasOnGround) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	Vec3 soundPos         = p->Base.next.pos;

	SoundComp_GetSound(p);
	if (!sounds_anyNonAir) soundPos = Vec3_BigPos();

	if (p->Base.OnGround && (SoundComp_ShouldPlay(p, soundPos) || !wasOnGround)) {
		Audio_PlayStepSound(sounds_type);
		sounds_lastPos = soundPos;
	}
}
