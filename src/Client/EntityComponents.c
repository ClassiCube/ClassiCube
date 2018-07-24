#include "EntityComponents.h"
#include "ExtMath.h"
#include "World.h"
#include "Block.h"
#include "Event.h"
#include "Game.h"
#include "Entity.h"
#include "Platform.h"
#include "Camera.h"
#include "Funcs.h"
#include "VertexStructs.h"
#include "GraphicsAPI.h"
#include "GraphicsCommon.h"
#include "ModelCache.h"
#include "Physics.h"
#include "IModel.h"
#include "Audio.h"

/*########################################################################################################################*
*----------------------------------------------------AnimatedComponent----------------------------------------------------*
*#########################################################################################################################*/
#define ANIM_MAX_ANGLE (110 * MATH_DEG2RAD)
#define ANIM_ARM_MAX (60.0f * MATH_DEG2RAD)
#define ANIM_LEG_MAX (80.0f * MATH_DEG2RAD)
#define ANIM_IDLE_MAX (3.0f * MATH_DEG2RAD)
#define ANIM_IDLE_XPERIOD (2.0f * MATH_PI / 5.0f)
#define ANIM_IDLE_ZPERIOD (2.0f * MATH_PI / 3.5f)

static void AnimatedComp_DoTilt(Real32* tilt, bool reduce) {
	if (reduce) {
		(*tilt) *= 0.84f;
	} else {
		(*tilt) += 0.1f;
	}
	Math_Clamp(*tilt, 0.0f, 1.0f);
}

static void AnimatedComp_PerpendicularAnim(struct AnimatedComp* anim, Real32 flapSpeed, Real32 idleXRot, Real32 idleZRot, bool left) {
	Real32 verAngle = 0.5f + 0.5f * Math_SinF(anim->WalkTime * flapSpeed);
	Real32 zRot = -idleZRot - verAngle * anim->Swing * ANIM_MAX_ANGLE;
	Real32 horAngle = Math_CosF(anim->WalkTime) * anim->Swing * ANIM_ARM_MAX * 1.5f;
	Real32 xRot = idleXRot + horAngle;

	if (left) {
		anim->LeftArmX = xRot;  anim->LeftArmZ = zRot;
	} else {
		anim->RightArmX = xRot; anim->RightArmZ = zRot;
	}
}

static void AnimatedComp_CalcHumanAnim(struct AnimatedComp* anim, Real32 idleXRot, Real32 idleZRot) {
	AnimatedComp_PerpendicularAnim(anim, 0.23f, idleXRot, idleZRot, true);
	AnimatedComp_PerpendicularAnim(anim, 0.28f, idleXRot, idleZRot, false);
	anim->RightArmX = -anim->RightArmX; anim->RightArmZ = -anim->RightArmZ;
}

void AnimatedComp_Init(struct AnimatedComp* anim) {
	Platform_MemSet(anim, 0, sizeof(struct AnimatedComp));
	anim->BobStrength = 1.0f; anim->BobStrengthO = 1.0f; anim->BobStrengthN = 1.0f;
}

void AnimatedComp_Update(struct Entity* entity, Vector3 oldPos, Vector3 newPos, Real64 delta) {
	struct AnimatedComp* anim = &entity->Anim;
	anim->WalkTimeO = anim->WalkTimeN;
	anim->SwingO    = anim->SwingN;
	Real32 dx = newPos.X - oldPos.X;
	Real32 dz = newPos.Z - oldPos.Z;
	Real32 distance = Math_SqrtF(dx * dx + dz * dz);

	if (distance > 0.05f) {
		Real32 walkDelta = distance * 2 * (Real32)(20 * delta);
		anim->WalkTimeN += walkDelta;
		anim->SwingN += (Real32)delta * 3;
	} else {
		anim->SwingN -= (Real32)delta * 3;
	}
	Math_Clamp(anim->SwingN, 0.0f, 1.0f);

	/* TODO: the Tilt code was designed for 60 ticks/second, fix it up for 20 ticks/second */
	anim->BobStrengthO = anim->BobStrengthN;
	Int32 i;
	for (i = 0; i < 3; i++) {
		AnimatedComp_DoTilt(&anim->BobStrengthN, !Game_ViewBobbing || !entity->OnGround);
	}
}

void AnimatedComp_GetCurrent(struct Entity* entity, Real32 t) {
	struct AnimatedComp* anim = &entity->Anim;
	anim->Swing = Math_Lerp(anim->SwingO, anim->SwingN, t);
	anim->WalkTime = Math_Lerp(anim->WalkTimeO, anim->WalkTimeN, t);
	anim->BobStrength = Math_Lerp(anim->BobStrengthO, anim->BobStrengthN, t);

	Real32 idleTime = (Real32)Game_Accumulator;
	Real32 idleXRot = Math_SinF(idleTime * ANIM_IDLE_XPERIOD) * ANIM_IDLE_MAX;
	Real32 idleZRot = ANIM_IDLE_MAX + Math_CosF(idleTime * ANIM_IDLE_ZPERIOD) * ANIM_IDLE_MAX;

	anim->LeftArmX = (Math_CosF(anim->WalkTime)  * anim->Swing * ANIM_ARM_MAX) - idleXRot;
	anim->LeftArmZ = -idleZRot;
	anim->LeftLegX = -(Math_CosF(anim->WalkTime) * anim->Swing * ANIM_LEG_MAX);
	anim->LeftLegZ = 0;

	anim->RightLegX = -anim->LeftLegX; anim->RightLegZ = -anim->LeftLegZ;
	anim->RightArmX = -anim->LeftArmX; anim->RightArmZ = -anim->LeftArmZ;

	anim->BobbingHor   = Math_CosF(anim->WalkTime)            * anim->Swing * (2.5f / 16.0f);
	anim->BobbingVer   = Math_AbsF(Math_SinF(anim->WalkTime)) * anim->Swing * (2.5f / 16.0f);
	anim->BobbingModel = Math_AbsF(Math_CosF(anim->WalkTime)) * anim->Swing * (4.0f / 16.0f);

	if (entity->Model->CalcHumanAnims && !Game_SimpleArmsAnim) {
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

void TiltComp_Update(struct TiltComp* anim, Real64 delta) {
	anim->VelTiltStrengthO = anim->VelTiltStrengthN;
	struct LocalPlayer* p = &LocalPlayer_Instance;

	/* TODO: the Tilt code was designed for 60 ticks/second, fix it up for 20 ticks/second */
	Int32 i;
	for (i = 0; i < 3; i++) {
		AnimatedComp_DoTilt(&anim->VelTiltStrengthN, p->Hacks.Floating);
	}
}

void TiltComp_GetCurrent(struct TiltComp* anim, Real32 t) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	anim->VelTiltStrength = Math_Lerp(anim->VelTiltStrengthO, anim->VelTiltStrengthN, t);

	struct AnimatedComp* pAnim = &p->Base.Anim;
	anim->TiltX = Math_CosF(pAnim->WalkTime) * pAnim->Swing * (0.15f * MATH_DEG2RAD);
	anim->TiltY = Math_SinF(pAnim->WalkTime) * pAnim->Swing * (0.15f * MATH_DEG2RAD);
}


/*########################################################################################################################*
*-----------------------------------------------------HacksComponent------------------------------------------------------*
*#########################################################################################################################*/
static void HacksComp_SetAll(struct HacksComp* hacks, bool allowed) {
	hacks->CanAnyHacks = allowed; hacks->CanFly = allowed;
	hacks->CanNoclip = allowed; hacks->CanRespawn = allowed;
	hacks->CanSpeed = allowed; hacks->CanPushbackBlocks = allowed;
	hacks->CanUseThirdPersonCamera = allowed;
}

void HacksComp_Init(struct HacksComp* hacks) {
	Platform_MemSet(hacks, 0, sizeof(struct HacksComp));
	HacksComp_SetAll(hacks, true);
	hacks->SpeedMultiplier = 10.0f;
	hacks->Enabled = true;
	hacks->CanSeeAllNames = true;
	hacks->CanDoubleJump = true;
	hacks->BaseHorSpeed = 1.0f;
	hacks->MaxJumps = 1;
	hacks->NoclipSlide = true;
	hacks->CanBePushed = true;
	hacks->HacksFlags = String_InitAndClearArray(hacks->HacksFlagsBuffer);
}

bool HacksComp_CanJumpHigher(struct HacksComp* hacks) {
	return hacks->Enabled && hacks->CanAnyHacks && hacks->CanSpeed;
}

bool HacksComp_Floating(struct HacksComp* hacks) {
	return hacks->Noclip || hacks->Flying;
}

static String HacksComp_UNSAFE_FlagValue(const UChar* flagRaw, struct HacksComp* hacks) {
	String* joined = &hacks->HacksFlags;
	String flag = String_FromReadonly(flagRaw);

	Int32 start = String_IndexOfString(joined, &flag);
	if (start < 0) return String_MakeNull();
	start += flag.length;

	Int32 end = String_IndexOf(joined, ' ', start);
	if (end < 0) end = joined->length;

	return String_UNSAFE_Substring(joined, start, end - start);
}

static Real32 HacksComp_ParseFlagReal(const UChar* flagRaw, struct HacksComp* hacks) {
	String raw = HacksComp_UNSAFE_FlagValue(flagRaw, hacks);
	if (!raw.length || Game_ClassicMode) return 1.0f;

	Real32 value = 0.0f;
	if (!Convert_TryParseReal32(&raw, &value)) return 1.0f;
	return value;
}

static Int32 HacksComp_ParseFlagInt(const UChar* flagRaw, struct HacksComp* hacks) {
	String raw = HacksComp_UNSAFE_FlagValue(flagRaw, hacks);
	if (!raw.length || Game_ClassicMode) return 1;

	Int32 value = 0;
	if (!Convert_TryParseInt32(&raw, &value)) return 1;
	return value;
}

static void HacksComp_ParseFlag(struct HacksComp* hacks, const UChar* incFlag, const UChar* excFlag, bool* target) {
	String include = String_FromReadonly(incFlag);
	String exclude = String_FromReadonly(excFlag);
	String* joined = &hacks->HacksFlags;

	if (String_ContainsString(joined, &include)) {
		*target = true;
	} else if (String_ContainsString(joined, &exclude)) {
		*target = false;
	}
}

static void HacksComp_ParseAllFlag(struct HacksComp* hacks, const UChar* incFlag, const UChar* excFlag) {
	String include = String_FromReadonly(incFlag);
	String exclude = String_FromReadonly(excFlag);
	String* joined = &hacks->HacksFlags;

	if (String_ContainsString(joined, &include)) {
		HacksComp_SetAll(hacks, true);
	} else if (String_ContainsString(joined, &exclude)) {
		HacksComp_SetAll(hacks, false);
	}
}

void HacksComp_SetUserType(struct HacksComp* hacks, UInt8 value, bool setBlockPerms) {
	bool isOp = value >= 100 && value <= 127;
	hacks->UserType = value;
	hacks->CanSeeAllNames = isOp;
	if (!setBlockPerms) return;

	Block_CanPlace[BLOCK_BEDROCK]     = isOp;
	Block_CanDelete[BLOCK_BEDROCK]    = isOp;
	Block_CanPlace[BLOCK_WATER]       = isOp;
	Block_CanPlace[BLOCK_STILL_WATER] = isOp;
	Block_CanPlace[BLOCK_LAVA]        = isOp;
	Block_CanPlace[BLOCK_STILL_LAVA]  = isOp;
}

void HacksComp_CheckConsistency(struct HacksComp* hacks) {
	if (!hacks->CanFly || !hacks->Enabled) {
		hacks->Flying = false; hacks->FlyingDown = false; hacks->FlyingUp = false;
	}
	if (!hacks->CanNoclip || !hacks->Enabled) {
		hacks->Noclip = false;
	}
	if (!hacks->CanSpeed || !hacks->Enabled) {
		hacks->Speeding = false; hacks->HalfSpeeding = false;
	}

	hacks->CanDoubleJump = hacks->CanAnyHacks && hacks->Enabled && hacks->CanSpeed;
	hacks->CanSeeAllNames = hacks->CanAnyHacks && hacks->CanSeeAllNames;

	if (!hacks->CanUseThirdPersonCamera || !hacks->Enabled) {
		Camera_CycleActive();
	}
}

void HacksComp_UpdateState(struct HacksComp* hacks) {
	HacksComp_SetAll(hacks, true);
	if (!hacks->HacksFlags.length) return;
	hacks->CanBePushed = true;

	/* By default (this is also the case with WoM), we can use hacks. */
	String excHacks = String_FromConst("-hax");
	if (String_ContainsString(&hacks->HacksFlags, &excHacks)) {
		HacksComp_SetAll(hacks, false);
	}

	HacksComp_ParseFlag(hacks, "+fly", "-fly", &hacks->CanFly);
	HacksComp_ParseFlag(hacks, "+noclip", "-noclip", &hacks->CanNoclip);
	HacksComp_ParseFlag(hacks, "+speed", "-speed", &hacks->CanSpeed);
	HacksComp_ParseFlag(hacks, "+respawn", "-respawn", &hacks->CanRespawn);
	HacksComp_ParseFlag(hacks, "+push", "-push", &hacks->CanBePushed);

	if (hacks->UserType == 0x64) {
		HacksComp_ParseAllFlag(hacks, "+ophax", "-ophax");
	}

	hacks->BaseHorSpeed  = HacksComp_ParseFlagReal("horspeed=", hacks);
	hacks->MaxJumps = HacksComp_ParseFlagInt("jumps=", hacks);

	HacksComp_CheckConsistency(hacks);
	Event_RaiseVoid(&UserEvents_HackPermissionsChanged);
}


/*########################################################################################################################*
*--------------------------------------------------InterpolationComponent-------------------------------------------------*
*#########################################################################################################################*/
static void InterpComp_RemoveOldestRotY(struct InterpComp* interp) {
	Int32 i;
	for (i = 0; i < Array_Elems(interp->RotYStates); i++) {
		interp->RotYStates[i] = interp->RotYStates[i + 1];
	}
	interp->RotYCount--;
}

static void InterpComp_AddRotY(struct InterpComp* interp, Real32 state) {
	if (interp->RotYCount == Array_Elems(interp->RotYStates)) {
		InterpComp_RemoveOldestRotY(interp);
	}
	interp->RotYStates[interp->RotYCount] = state; interp->RotYCount++;
}

static void InterpComp_AdvanceRotY(struct InterpComp* interp) {
	interp->PrevRotY = interp->NextRotY;
	if (!interp->RotYCount) return;

	interp->NextRotY = interp->RotYStates[0];
	InterpComp_RemoveOldestRotY(interp);
}

void InterpComp_LerpAngles(struct InterpComp* interp, struct Entity* entity, Real32 t) {
	struct InterpState* prev = &interp->Prev;
	struct InterpState* next = &interp->Next;

	entity->HeadX = Math_LerpAngle(prev->HeadX, next->HeadX, t);
	entity->HeadY = Math_LerpAngle(prev->HeadY, next->HeadY, t);
	entity->RotX  = Math_LerpAngle(prev->RotX,  next->RotX, t);
	entity->RotY  = Math_LerpAngle(interp->PrevRotY, interp->NextRotY, t);
	entity->RotZ  = Math_LerpAngle(prev->RotZ,  next->RotZ, t);
}

static void InterpComp_SetPos(struct InterpState* state, struct LocationUpdate* update) {
	if (update->RelativePos) {
		Vector3_AddBy(&state->Pos, &update->Pos);
	} else {
		state->Pos = update->Pos;
	}
}


/*########################################################################################################################*
*----------------------------------------------NetworkInterpolationComponent----------------------------------------------*
*#########################################################################################################################*/
static void NetInterpComp_RemoveOldestState(struct NetInterpComp* interp) {
	Int32 i;
	for (i = 0; i < Array_Elems(interp->States); i++) {
		interp->States[i] = interp->States[i + 1];
	}
	interp->StatesCount--;
}

static void NetInterpComp_AddState(struct NetInterpComp* interp, struct InterpState state) {
	if (interp->StatesCount == Array_Elems(interp->States)) {
		NetInterpComp_RemoveOldestState(interp);
	}
	interp->States[interp->StatesCount] = state; interp->StatesCount++;
}

void NetInterpComp_SetLocation(struct NetInterpComp* interp, struct LocationUpdate* update, bool interpolate) {
	struct InterpState last = interp->Cur;
	struct InterpState* cur = &interp->Cur;
	UInt8 flags = update->Flags;

	if (flags & LOCATIONUPDATE_FLAG_POS)   InterpComp_SetPos(cur, update);
	if (flags & LOCATIONUPDATE_FLAG_ROTX)  cur->RotX  = update->RotX;
	if (flags & LOCATIONUPDATE_FLAG_ROTZ)  cur->RotZ  = update->RotZ;
	if (flags & LOCATIONUPDATE_FLAG_HEADX) cur->HeadX = update->HeadX;
	if (flags & LOCATIONUPDATE_FLAG_HEADY) cur->HeadY = update->HeadY;

	if (!interpolate) {
		interp->Prev = *cur; interp->PrevRotY = cur->HeadY;
		interp->Next = *cur; interp->NextRotY = cur->HeadY;
		interp->RotYCount = 0; interp->StatesCount = 0;
	} else {
		/* Smoother interpolation by also adding midpoint. */
		struct InterpState mid;
		Vector3_Lerp(&mid.Pos, &last.Pos, &cur->Pos, 0.5f);
		mid.RotX  = Math_LerpAngle(last.RotX,  cur->RotX,  0.5f);
		mid.RotZ  = Math_LerpAngle(last.RotZ,  cur->RotZ,  0.5f);
		mid.HeadX = Math_LerpAngle(last.HeadX, cur->HeadX, 0.5f);
		mid.HeadY = Math_LerpAngle(last.HeadY, cur->HeadY, 0.5f);
		NetInterpComp_AddState(interp, mid);
		NetInterpComp_AddState(interp, *cur);

		/* Head rotation lags behind body a tiny bit */
		InterpComp_AddRotY((struct InterpComp*)interp, Math_LerpAngle(last.HeadY, cur->HeadY, 0.33333333f));
		InterpComp_AddRotY((struct InterpComp*)interp, Math_LerpAngle(last.HeadY, cur->HeadY, 0.66666667f));
		InterpComp_AddRotY((struct InterpComp*)interp, Math_LerpAngle(last.HeadY, cur->HeadY, 1.00000000f));
	}
}

void NetInterpComp_AdvanceState(struct NetInterpComp* interp) {
	interp->Prev = interp->Next;
	if (interp->StatesCount > 0) {
		interp->Next = interp->States[0];
		NetInterpComp_RemoveOldestState(interp);
	}
	InterpComp_AdvanceRotY((struct InterpComp*)interp);
}


/*########################################################################################################################*
*-----------------------------------------------LocalInterpolationComponent-----------------------------------------------*
*#########################################################################################################################*/
static void LocalInterpComp_Angle(Real32* prev, Real32* next, Real32 value, bool interpolate) {
	*next = value;
	if (!interpolate) *prev = value;
}

void LocalInterpComp_SetLocation(struct InterpComp* interp, struct LocationUpdate* update, bool interpolate) {
	struct Entity* entity = &LocalPlayer_Instance.Base;
	struct InterpState* prev = &interp->Prev;
	struct InterpState* next = &interp->Next;
	UInt8 flags = update->Flags;

	if (flags & LOCATIONUPDATE_FLAG_POS) {
		InterpComp_SetPos(next, update);
		/* If server sets Y position exactly on ground, push up a tiny bit */
		Real32 yOffset = next->Pos.Y - Math_Floor(next->Pos.Y);
		if (yOffset < ENTITY_ADJUSTMENT) { next->Pos.Y += ENTITY_ADJUSTMENT; }
		if (!interpolate) { prev->Pos = next->Pos; entity->Position = next->Pos; }
	}

	if (flags & LOCATIONUPDATE_FLAG_HEADX) {
		LocalInterpComp_Angle(&prev->HeadX, &next->HeadX, update->HeadX, interpolate);
	}
	if (flags & LOCATIONUPDATE_FLAG_HEADY) {
		LocalInterpComp_Angle(&prev->HeadY, &next->HeadY, update->HeadY, interpolate);
	}
	if (flags & LOCATIONUPDATE_FLAG_ROTX) {
		LocalInterpComp_Angle(&prev->RotX, &next->RotX, update->RotX, interpolate);
	}
	if (flags & LOCATIONUPDATE_FLAG_ROTZ) {
		LocalInterpComp_Angle(&prev->RotZ, &next->RotZ, update->RotZ, interpolate);
	}

	if (flags & LOCATIONUPDATE_FLAG_HEADY) {
		if (!interpolate) {
			interp->NextRotY = update->HeadY;
			entity->RotY     = update->HeadY;
			interp->RotYCount = 0;
		} else {
			/* Body Y rotation lags slightly behind */
			InterpComp_AddRotY(interp, Math_LerpAngle(prev->HeadY, next->HeadY, 0.33333333f));
			InterpComp_AddRotY(interp, Math_LerpAngle(prev->HeadY, next->HeadY, 0.66666667f));
			InterpComp_AddRotY(interp, Math_LerpAngle(prev->HeadY, next->HeadY, 1.00000000f));

			interp->NextRotY = interp->RotYStates[0];
		}
	}
	InterpComp_LerpAngles(interp, entity, 0.0f);
}

void LocalInterpComp_AdvanceState(struct InterpComp* interp) {
	struct Entity* entity = &LocalPlayer_Instance.Base;
	interp->Prev = interp->Next;
	entity->Position = interp->Next.Pos;
	InterpComp_AdvanceRotY(interp);
}


/*########################################################################################################################*
*-----------------------------------------------------ShadowComponent-----------------------------------------------------*
*#########################################################################################################################*/
Real32 ShadowComponent_radius, shadowComponent_uvScale;
struct ShadowData { Real32 Y; BlockID Block; UInt8 A; };

bool lequal(Real32 a, Real32 b) { return a < b || Math_AbsF(a - b) < 0.001f; }
static void ShadowComponent_DrawCoords(VertexP3fT2fC4b** vertices, struct Entity* entity, struct ShadowData* data, Real32 x1, Real32 z1, Real32 x2, Real32 z2) {
	Vector3 cen = entity->Position;
	if (lequal(x2, x1) || lequal(z2, z1)) return;

	Real32 u1 = (x1 - cen.X) * shadowComponent_uvScale + 0.5f;
	Real32 v1 = (z1 - cen.Z) * shadowComponent_uvScale + 0.5f;
	Real32 u2 = (x2 - cen.X) * shadowComponent_uvScale + 0.5f;
	Real32 v2 = (z2 - cen.Z) * shadowComponent_uvScale + 0.5f;
	if (u2 <= 0.0f || v2 <= 0.0f || u1 >= 1.0f || v1 >= 1.0f) return;

	x1 = max(x1, cen.X - ShadowComponent_radius); u1 = u1 >= 0.0f ? u1 : 0.0f;
	z1 = max(z1, cen.Z - ShadowComponent_radius); v1 = v1 >= 0.0f ? v1 : 0.0f;
	x2 = min(x2, cen.X + ShadowComponent_radius); u2 = u2 <= 1.0f ? u2 : 1.0f;
	z2 = min(z2, cen.Z + ShadowComponent_radius); v2 = v2 <= 1.0f ? v2 : 1.0f;

	PackedCol col = PACKEDCOL_CONST(255, 255, 255, data->A);
	VertexP3fT2fC4b* ptr = *vertices;
	VertexP3fT2fC4b v; v.Y = data->Y; v.Col = col;

	v.X = x1; v.Z = z1; v.U = u1; v.V = v1; *ptr++ = v;
	v.X = x2;           v.U = u2;           *ptr++ = v;
	          v.Z = z2;           v.V = v2; *ptr++ = v;
	v.X = x1;           v.U = u1;           *ptr++ = v;

	*vertices = ptr;
}

static void ShadowComponent_DrawSquareShadow(VertexP3fT2fC4b** vertices, Real32 y, Real32 x, Real32 z) {
	PackedCol col = PACKEDCOL_CONST(255, 255, 255, 220);
	struct TextureRec rec = { 63.0f / 128.0f, 63.0f / 128.0f, 64.0f / 128.0f, 64.0f / 128.0f };
	VertexP3fT2fC4b* ptr = *vertices;
	VertexP3fT2fC4b v; v.Y = y; v.Col = col;

	v.X = x;        v.Z = z;        v.U = rec.U1; v.V = rec.V1; *ptr = v; ptr++;
	v.X = x + 1.0f;                 v.U = rec.U2;               *ptr = v; ptr++;
	                v.Z = z + 1.0f;               v.V = rec.V2; *ptr = v; ptr++;
	v.X = x;                        v.U = rec.U1;               *ptr = v; ptr++;

	*vertices = ptr;
}

static void ShadowComponent_DrawCircle(VertexP3fT2fC4b** vertices, struct Entity* entity, struct ShadowData* data, Real32 x, Real32 z) {
	x = (Real32)Math_Floor(x); z = (Real32)Math_Floor(z);
	Vector3 min = Block_MinBB[data[0].Block], max = Block_MaxBB[data[0].Block];

	ShadowComponent_DrawCoords(vertices, entity, &data[0], x + min.X, z + min.Z, x + max.X, z + max.Z);
	UInt32 i;
	for (i = 1; i < 4; i++) {
		if (data[i].Block == BLOCK_AIR) return;
		Vector3 nMin = Block_MinBB[data[i].Block], nMax = Block_MaxBB[data[i].Block];
		ShadowComponent_DrawCoords(vertices, entity, &data[i], x + min.X, z + nMin.Z, x + max.X, z + min.Z);
		ShadowComponent_DrawCoords(vertices, entity, &data[i], x + min.X, z + max.Z, x + max.X, z + nMax.Z);

		ShadowComponent_DrawCoords(vertices, entity, &data[i], x + nMin.X, z + nMin.Z, x + min.X, z + nMax.Z);
		ShadowComponent_DrawCoords(vertices, entity, &data[i], x + max.X, z + nMin.Z, x + nMax.X, z + nMax.Z);
		min = nMin; max = nMax;
	}
}

static void ShadowComponent_CalcAlpha(Real32 playerY, struct ShadowData* data) {
	Real32 height = playerY - data->Y;
	if (height <= 6.0f) {
		data->A = (UInt8)(160 - 160 * height / 6.0f);
		data->Y += 1.0f / 64.0f; return;
	}

	data->A = 0;
	if (height <= 16.0f)      data->Y += 1.0f / 64.0f;
	else if (height <= 32.0f) data->Y += 1.0f / 16.0f;
	else if (height <= 96.0f) data->Y += 1.0f / 8.0f;
	else data->Y += 1.0f / 4.0f;
}

static bool ShadowComponent_GetBlocks(struct Entity* entity, Int32 x, Int32 y, Int32 z, struct ShadowData* data) {
	Int32 count;
	struct ShadowData zeroData = { 0 };
	for (count = 0; count < 4; count++) { data[count] = zeroData; }
	count = 0;

	struct ShadowData* cur = data;
	Real32 posY = entity->Position.Y;
	bool outside = x < 0 || z < 0 || x >= World_Width || z >= World_Length;

	while (y >= 0 && count < 4) {
		BlockID block;
		if (!outside) {
			block = World_GetBlock(x, y, z);
		} else if (y == WorldEnv_EdgeHeight - 1) {
			block = Block_Draw[WorldEnv_EdgeBlock] == DRAW_GAS  ? BLOCK_AIR : BLOCK_BEDROCK;
		} else if (y == WorldEnv_SidesHeight - 1) {
			block = Block_Draw[WorldEnv_SidesBlock] == DRAW_GAS ? BLOCK_AIR : BLOCK_BEDROCK;
		} else {
			block = BLOCK_AIR;
		}
		y--;

		UInt8 draw = Block_Draw[block];
		if (draw == DRAW_GAS || draw == DRAW_SPRITE || Block_IsLiquid[block]) continue;
		Real32 blockY = (y + 1.0f) + Block_MaxBB[block].Y;
		if (blockY >= posY + 0.01f) continue;

		cur->Block = block; cur->Y = blockY;
		ShadowComponent_CalcAlpha(posY, cur);
		count++; cur++;

		/* Check if the casted shadow will continue on further down. */
		if (Block_MinBB[block].X == 0.0f && Block_MaxBB[block].X == 1.0f &&
			Block_MinBB[block].Z == 0.0f && Block_MaxBB[block].Z == 1.0f) return true;
	}

	if (count < 4) {
		cur->Block = WorldEnv_EdgeBlock; cur->Y = 0.0f;
		ShadowComponent_CalcAlpha(posY, cur);
		count++; cur++;
	}
	return true;
}

#define sh_size 128
#define sh_half (sh_size / 2)
static void ShadowComponent_MakeTex(void) {
	UInt8 pixels[Bitmap_DataSize(sh_size, sh_size)];
	struct Bitmap bmp; Bitmap_Create(&bmp, sh_size, sh_size, pixels);

	UInt32 inPix  = PackedCol_ARGB(0, 0, 0, 200);
	UInt32 outPix = PackedCol_ARGB(0, 0, 0, 0);

	UInt32 x, y;
	for (y = 0; y < sh_size; y++) {
		UInt32* row = Bitmap_GetRow(&bmp, y);
		for (x = 0; x < sh_size; x++) {
			Real64 dist = 
				(sh_half - (x + 0.5)) * (sh_half - (x + 0.5)) +
				(sh_half - (y + 0.5)) * (sh_half - (y + 0.5));
			row[x] = dist < sh_half * sh_half ? inPix : outPix;
		}
	}
	ShadowComponent_ShadowTex = Gfx_CreateTexture(&bmp, false, false);
}

void ShadowComponent_Draw(struct Entity* entity) {
	Vector3 Position = entity->Position;
	if (Position.Y < 0.0f) return;

	Real32 posX = Position.X, posZ = Position.Z;
	Int32 posY = min((Int32)Position.Y, World_MaxY);
	GfxResourceID vb;

	Real32 radius = 7.0f * min(entity->ModelScale.Y, 1.0f) * entity->Model->ShadowScale;
	ShadowComponent_radius = radius / 16.0f;
	shadowComponent_uvScale = 16.0f / (radius * 2.0f);

	VertexP3fT2fC4b vertices[128];
	struct ShadowData data[4];

	/* TODO: Should shadow component use its own VB? */
	VertexP3fT2fC4b* ptr = vertices;
	if (Entities_ShadowMode == SHADOW_MODE_SNAP_TO_BLOCK) {
		vb = GfxCommon_texVb;
		Int32 x1 = Math_Floor(posX), z1 = Math_Floor(posZ);
		if (!ShadowComponent_GetBlocks(entity, x1, posY, z1, data)) return;

		ShadowComponent_DrawSquareShadow(&ptr, data[0].Y, x1, z1);
	} else {
		vb = ModelCache_Vb;
		Int32 x1 = Math_Floor(posX - ShadowComponent_radius), z1 = Math_Floor(posZ - ShadowComponent_radius);
		Int32 x2 = Math_Floor(posX + ShadowComponent_radius), z2 = Math_Floor(posZ + ShadowComponent_radius);

		if (ShadowComponent_GetBlocks(entity, x1, posY, z1, data) && data[0].A > 0) {
			ShadowComponent_DrawCircle(&ptr, entity, data, (Real32)x1, (Real32)z1);
		}
		if (x1 != x2 && ShadowComponent_GetBlocks(entity, x2, posY, z1, data) && data[0].A > 0) {
			ShadowComponent_DrawCircle(&ptr, entity, data, (Real32)x2, (Real32)z1);
		}
		if (z1 != z2 && ShadowComponent_GetBlocks(entity, x1, posY, z2, data) && data[0].A > 0) {
			ShadowComponent_DrawCircle(&ptr, entity, data, (Real32)x1, (Real32)z2);
		}
		if (x1 != x2 && z1 != z2 && ShadowComponent_GetBlocks(entity, x2, posY, z2, data) && data[0].A > 0) {
			ShadowComponent_DrawCircle(&ptr, entity, data, (Real32)x2, (Real32)z2);
		}
	}

	if (ptr == vertices) return;
	if (!ShadowComponent_ShadowTex) ShadowComponent_MakeTex();

	if (!ShadowComponent_BoundShadowTex) {
		Gfx_BindTexture(ShadowComponent_ShadowTex);
		ShadowComponent_BoundShadowTex = true;
	}

	UInt32 vCount = (UInt32)(ptr - vertices);
	GfxCommon_UpdateDynamicVb_IndexedTris(vb, vertices, vCount);
}


/*########################################################################################################################*
*---------------------------------------------------CollisionsComponent---------------------------------------------------*
*#########################################################################################################################*/
/* Whether a collision occurred with any horizontal sides of any blocks */
bool Collisions_HitHorizontal(struct CollisionsComp* comp) {
	return comp->HitXMin || comp->HitXMax || comp->HitZMin || comp->HitZMax;
}
#define COLLISIONS_ADJ 0.001f

static void Collisions_ClipX(struct Entity* entity, Vector3* size, struct AABB* entityBB, struct AABB* extentBB) {
	entity->Velocity.X = 0.0f;
	entityBB->Min.X = entity->Position.X - size->X / 2; extentBB->Min.X = entityBB->Min.X;
	entityBB->Max.X = entity->Position.X + size->X / 2; extentBB->Max.X = entityBB->Max.X;
}

static void Collisions_ClipY(struct Entity* entity, Vector3* size, struct AABB* entityBB, struct AABB* extentBB) {
	entity->Velocity.Y = 0.0f;
	entityBB->Min.Y = entity->Position.Y;               extentBB->Min.Y = entityBB->Min.Y;
	entityBB->Max.Y = entity->Position.Y + size->Y;     extentBB->Max.Y = entityBB->Max.Y;
}

static void Collisions_ClipZ(struct Entity* entity, Vector3* size, struct AABB* entityBB, struct AABB* extentBB) {
	entity->Velocity.Z = 0.0f;
	entityBB->Min.Z = entity->Position.Z - size->Z / 2; extentBB->Min.Z = entityBB->Min.Z;
	entityBB->Max.Z = entity->Position.Z + size->Z / 2; extentBB->Max.Z = entityBB->Max.Z;
}

static bool Collisions_CanSlideThrough(struct AABB* adjFinalBB) {
	Vector3I bbMin; Vector3I_Floor(&bbMin, &adjFinalBB->Min);
	Vector3I bbMax; Vector3I_Floor(&bbMax, &adjFinalBB->Max);

	struct AABB blockBB;
	Vector3 v;
	Int32 x, y, z;
	for (y = bbMin.Y; y <= bbMax.Y; y++) { v.Y = (Real32)y;
		for (z = bbMin.Z; z <= bbMax.Z; z++) { v.Z = (Real32)z;
			for (x = bbMin.X; x <= bbMax.X; x++) { v.X = (Real32)x;
				BlockID block = World_GetPhysicsBlock(x, y, z);
				Vector3_Add(&blockBB.Min, &v, &Block_MinBB[block]);
				Vector3_Add(&blockBB.Max, &v, &Block_MaxBB[block]);

				if (!AABB_Intersects(&blockBB, adjFinalBB)) continue;
				if (Block_Collide[block] == COLLIDE_SOLID) return false;
			}
		}
	}
	return true;
}

static bool Collisions_DidSlide(struct CollisionsComp* comp, struct AABB* blockBB, Vector3* size,
	struct AABB* finalBB, struct AABB* entityBB, struct AABB* extentBB) {
	Real32 yDist = blockBB->Max.Y - entityBB->Min.Y;
	if (yDist > 0.0f && yDist <= comp->Entity->StepSize + 0.01f) {
		Real32 blockBB_MinX = max(blockBB->Min.X, blockBB->Max.X - size->X / 2);
		Real32 blockBB_MaxX = min(blockBB->Max.X, blockBB->Min.X + size->X / 2);
		Real32 blockBB_MinZ = max(blockBB->Min.Z, blockBB->Max.Z - size->Z / 2);
		Real32 blockBB_MaxZ = min(blockBB->Max.Z, blockBB->Min.Z + size->Z / 2);

		struct AABB adjBB;
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
	bool wasOn, struct AABB* finalBB, struct AABB* extentBB, Vector3* size) {
	if (!wasOn || !Collisions_DidSlide(comp, blockBB, size, finalBB, entityBB, extentBB)) {
		comp->Entity->Position.X = blockBB->Min.X - size->X / 2 - COLLISIONS_ADJ;
		Collisions_ClipX(comp->Entity, size, entityBB, extentBB);
		comp->HitXMin = true;
	}
}

static void Collisions_ClipXMax(struct CollisionsComp* comp, struct AABB* blockBB, struct AABB* entityBB, 
	bool wasOn, struct AABB* finalBB, struct AABB* extentBB, Vector3* size) {
	if (!wasOn || !Collisions_DidSlide(comp, blockBB, size, finalBB, entityBB, extentBB)) {
		comp->Entity->Position.X = blockBB->Max.X + size->X / 2 + COLLISIONS_ADJ;
		Collisions_ClipX(comp->Entity, size, entityBB, extentBB);
		comp->HitXMax = true;
	}
}

static void Collisions_ClipZMax(struct CollisionsComp* comp, struct AABB* blockBB, struct AABB* entityBB, 
	bool wasOn, struct AABB* finalBB, struct AABB* extentBB, Vector3* size) {
	if (!wasOn || !Collisions_DidSlide(comp, blockBB, size, finalBB, entityBB, extentBB)) {
		comp->Entity->Position.Z = blockBB->Max.Z + size->Z / 2 + COLLISIONS_ADJ;
		Collisions_ClipZ(comp->Entity, size, entityBB, extentBB);
		comp->HitZMax = true;
	}
}

static void Collisions_ClipZMin(struct CollisionsComp* comp, struct AABB* blockBB, struct AABB* entityBB,
	bool wasOn, struct AABB* finalBB, struct AABB* extentBB, Vector3* size) {
	if (!wasOn || !Collisions_DidSlide(comp, blockBB, size, finalBB, entityBB, extentBB)) {
		comp->Entity->Position.Z = blockBB->Min.Z - size->Z / 2 - COLLISIONS_ADJ;
		Collisions_ClipZ(comp->Entity, size, entityBB, extentBB);
		comp->HitZMin = true;
	}
}

static void Collisions_ClipYMin(struct CollisionsComp* comp, struct AABB* blockBB, struct AABB* entityBB, 
	struct AABB* extentBB, Vector3* size) {
	comp->Entity->Position.Y = blockBB->Min.Y - size->Y - COLLISIONS_ADJ;
	Collisions_ClipY(comp->Entity, size, entityBB, extentBB);
	comp->HitYMin = true;
}

static void Collisions_ClipYMax(struct CollisionsComp* comp, struct AABB* blockBB, struct AABB* entityBB, 
	struct AABB* extentBB, Vector3* size) {
	comp->Entity->Position.Y = blockBB->Max.Y + COLLISIONS_ADJ;
	comp->Entity->OnGround = true;
	Collisions_ClipY(comp->Entity, size, entityBB, extentBB);
	comp->HitYMax = true;
}

static void Collisions_CollideWithReachableBlocks(struct CollisionsComp* comp, Int32 count, struct AABB* entityBB, 
	struct AABB* extentBB) {
	struct Entity* entity = comp->Entity;
	/* Reset collision detection states */
	bool wasOn = entity->OnGround;
	entity->OnGround = false;
	comp->HitXMin = false; comp->HitYMin = false; comp->HitZMin = false;
	comp->HitXMax = false; comp->HitYMax = false; comp->HitZMax = false;

	struct AABB blockBB;
	Vector3 bPos, size = entity->Size;
	Int32 i;
	for (i = 0; i < count; i++) {
		/* Unpack the block and coordinate data */
		struct SearcherState state = Searcher_States[i];
		bPos.X = state.X >> 3; bPos.Y = state.Y >> 3; bPos.Z = state.Z >> 3;
		Int32 block = (state.X & 0x7) | (state.Y & 0x7) << 3 | (state.Z & 0x7) << 6;

		Vector3_Add(&blockBB.Min, &Block_MinBB[block], &bPos);
		Vector3_Add(&blockBB.Max, &Block_MaxBB[block], &bPos);
		if (!AABB_Intersects(extentBB, &blockBB)) continue;

		/* Recheck time to collide with block (as colliding with blocks modifies this) */
		Real32 tx, ty, tz;
		Searcher_CalcTime(&entity->Velocity, entityBB, &blockBB, &tx, &ty, &tz);
		if (tx > 1.0f || ty > 1.0f || tz > 1.0f) {
			Platform_LogConst("t > 1 in physics calculation.. this shouldn't have happened.");
		}

		/* Calculate the location of the entity when it collides with this block */
		Vector3 v = entity->Velocity; 
		v.X *= tx; v.Y *= ty; v.Z *= tz;
		struct AABB finalBB; /* Inlined ABBB_Offset */
		Vector3_Add(&finalBB.Min, &entityBB->Min, &v);
		Vector3_Add(&finalBB.Max, &entityBB->Max, &v);

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
	Vector3 zero = Vector3_Zero;
	struct Entity* entity = comp->Entity;
	if (Vector3_Equals(&entity->Velocity, &zero)) return;

	struct AABB entityBB, entityExtentBB;
	Int32 count = Searcher_FindReachableBlocks(entity, &entityBB, &entityExtentBB);
	Collisions_CollideWithReachableBlocks(comp, count, &entityBB, &entityExtentBB);
}


/*########################################################################################################################*
*----------------------------------------------------PhysicsComponent-----------------------------------------------------*
*#########################################################################################################################*/
void PhysicsComp_Init(struct PhysicsComp* comp, struct Entity* entity) {
	Platform_MemSet(comp, 0, sizeof(struct PhysicsComp));
	comp->CanLiquidJump = true;
	comp->Entity = entity;
	comp->JumpVel       = 0.42f;
	comp->UserJumpVel   = 0.42f;
	comp->ServerJumpVel = 0.42f;
}

static bool PhysicsComp_TouchesLiquid(BlockID block) { return Block_Collide[block] == COLLIDE_LIQUID; }
void PhysicsComp_UpdateVelocityState(struct PhysicsComp* comp) {
	struct Entity* entity = comp->Entity;
	struct HacksComp* hacks = comp->Hacks;

	if (hacks->Floating) {
		entity->Velocity.Y = 0.0f; /* eliminate the effect of gravity */
		Int32 dir = (hacks->FlyingUp || comp->Jumping) ? 1 : (hacks->FlyingDown ? -1 : 0);

		entity->Velocity.Y += 0.12f * dir;
		if (hacks->Speeding     && hacks->CanSpeed) entity->Velocity.Y += 0.12f * dir;
		if (hacks->HalfSpeeding && hacks->CanSpeed) entity->Velocity.Y += 0.06f * dir;
	} else if (comp->Jumping && Entity_TouchesAnyRope(entity) && entity->Velocity.Y > 0.02f) {
		entity->Velocity.Y = 0.02f;
	}

	if (!comp->Jumping) {
		comp->CanLiquidJump = false; return;
	}

	bool touchWater = Entity_TouchesAnyWater(entity);
	bool touchLava  = Entity_TouchesAnyLava(entity);
	if (touchWater || touchLava) {
		struct AABB bounds; Entity_GetBounds(entity, &bounds);
		Int32 feetY = Math_Floor(bounds.Min.Y), bodyY = feetY + 1;
		Int32 headY = Math_Floor(bounds.Max.Y);
		if (bodyY > headY) bodyY = headY;

		bounds.Max.Y = bounds.Min.Y = feetY;
		bool liquidFeet = Entity_TouchesAny(&bounds, PhysicsComp_TouchesLiquid);
		bounds.Min.Y = min(bodyY, headY);
		bounds.Max.Y = max(bodyY, headY);
		bool liquidRest = Entity_TouchesAny(&bounds, PhysicsComp_TouchesLiquid);

		bool pastJumpPoint = liquidFeet && !liquidRest && (Math_Mod1(entity->Position.Y) >= 0.4f);
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
	struct Entity* entity = comp->Entity;
	struct HacksComp* hacks = comp->Hacks;
	if (comp->JumpVel == 0.0f || hacks->MaxJumps <= 0) return;

	entity->Velocity.Y = comp->JumpVel;
	if (hacks->Speeding     && hacks->CanSpeed) entity->Velocity.Y += comp->JumpVel;
	if (hacks->HalfSpeeding && hacks->CanSpeed) entity->Velocity.Y += comp->JumpVel / 2;
	comp->CanLiquidJump = false;
}

static bool PhysicsComp_TouchesSlipperyIce(BlockID b) { return Block_ExtendedCollide[b] == COLLIDE_SLIPPERY_ICE; }
static bool PhysicsComp_OnIce(struct Entity* entity) {
	Vector3 under = entity->Position; under.Y -= 0.01f;
	Vector3I underCoords; Vector3I_Floor(&underCoords, &under);
	BlockID blockUnder = World_SafeGetBlock_3I(underCoords);
	if (Block_ExtendedCollide[blockUnder] == COLLIDE_ICE) return true;

	struct AABB bounds; Entity_GetBounds(entity, &bounds);
	bounds.Min.Y -= 0.01f; bounds.Max.Y = bounds.Min.Y;
	return Entity_TouchesAny(&bounds, PhysicsComp_TouchesSlipperyIce);
}

static void PhysicsComp_MoveHor(struct PhysicsComp* comp, Vector3 vel, Real32 factor) {
	Real32 dist = Math_SqrtF(vel.X * vel.X + vel.Z * vel.Z);
	if (dist < 0.00001f) return;
	if (dist < 1.0f) dist = 1.0f;

	/* entity.Velocity += vel * (factor / dist) */
	struct Entity* entity = comp->Entity;
	Vector3_Mul1By(&vel, factor / dist);
	Vector3_AddBy(&entity->Velocity, &vel);
}

static void PhysicsComp_Move(struct PhysicsComp* comp, Vector3 drag, Real32 gravity, Real32 yMul) {
	struct Entity* entity = comp->Entity;
	entity->Velocity.Y *= yMul;
	if (!comp->Hacks->Noclip) {
		Collisions_MoveAndWallSlide(comp->Collisions);
	}
	Vector3_AddBy(&entity->Position, &entity->Velocity);

	entity->Velocity.Y /= yMul;
	Vector3_Mul3By(&entity->Velocity, &drag);
	entity->Velocity.Y -= gravity;
}

static void PhysicsComp_MoveFlying(struct PhysicsComp* comp, Vector3 vel, Real32 factor, Vector3 drag, Real32 gravity, Real32 yMul) {
	struct Entity* entity = comp->Entity;
	struct HacksComp* hacks = comp->Hacks;

	PhysicsComp_MoveHor(comp, vel, factor);
	Real32 yVel = Math_SqrtF(entity->Velocity.X * entity->Velocity.X + entity->Velocity.Z * entity->Velocity.Z);
	/* make horizontal speed the same as vertical speed */
	if ((vel.X != 0.0f || vel.Z != 0.0f) && yVel > 0.001f) {
		entity->Velocity.Y = 0.0f;
		yMul = 1.0f;
		if (hacks->FlyingUp || comp->Jumping) entity->Velocity.Y += yVel;
		if (hacks->FlyingDown)                entity->Velocity.Y -= yVel;
	}
	PhysicsComp_Move(comp, drag, gravity, yMul);
}

static void PhysicsComp_MoveNormal(struct PhysicsComp* comp, Vector3 vel, Real32 factor, Vector3 drag, Real32 gravity, Real32 yMul) {
	PhysicsComp_MoveHor(comp, vel, factor);
	PhysicsComp_Move(comp, drag, gravity, yMul);
}

static Real32 PhysicsComp_LowestModifier(struct PhysicsComp* comp, struct AABB* bounds, bool checkSolid) {
	Vector3I bbMin, bbMax;
	Vector3I_Floor(&bbMin, &bounds->Min);
	Vector3I_Floor(&bbMax, &bounds->Max);
	Real32 modifier = MATH_POS_INF;

	bbMin.X = max(bbMin.X, 0); bbMax.X = min(bbMax.X, World_MaxX);
	bbMin.Y = max(bbMin.Y, 0); bbMax.Y = min(bbMax.Y, World_MaxY);
	bbMin.Z = max(bbMin.Z, 0); bbMax.Z = min(bbMax.Z, World_MaxZ);

	struct AABB blockBB;
	Vector3 v;
	Int32 x, y, z;
	for (y = bbMin.Y; y <= bbMax.Y; y++) { v.Y = (Real32)y;
		for (z = bbMin.Z; z <= bbMax.Z; z++) { v.Z = (Real32)z;
			for (x = bbMin.X; x <= bbMax.X; x++) { v.X = (Real32)x;
				BlockID block = World_GetBlock(x, y, z);
				if (block == BLOCK_AIR) continue;
				UInt8 collide = Block_Collide[block];
				if (collide == COLLIDE_SOLID && !checkSolid) continue;

				Vector3_Add(&blockBB.Min, &v, &Block_MinBB[block]);
				Vector3_Add(&blockBB.Max, &v, &Block_MaxBB[block]);
				if (!AABB_Intersects(&blockBB, bounds)) continue;

				modifier = min(modifier, Block_SpeedMultiplier[block]);
				if (Block_ExtendedCollide[block] == COLLIDE_LIQUID) {
					comp->UseLiquidGravity = true;
				}
			}
		}
	}
	return modifier;
}

static Real32 PhysicsComp_GetSpeed(struct HacksComp* hacks, Real32 speedMul) {
	Real32 factor = hacks->Floating ? speedMul : 1.0f, speed = factor;
	if (hacks->Speeding     && hacks->CanSpeed) speed += factor * hacks->SpeedMultiplier;
	if (hacks->HalfSpeeding && hacks->CanSpeed) speed += factor * hacks->SpeedMultiplier / 2;
	return hacks->CanSpeed ? speed : min(speed, 1.0f);
}

static Real32 PhysicsComp_GetBaseSpeed(struct PhysicsComp* comp) {
	struct AABB bounds; Entity_GetBounds(comp->Entity, &bounds);
	comp->UseLiquidGravity = false;
	Real32 baseModifier = PhysicsComp_LowestModifier(comp, &bounds, false);
	bounds.Min.Y -= 0.5f / 16.0f; /* also check block standing on */
	Real32 solidModifier = PhysicsComp_LowestModifier(comp, &bounds, true);

	if (baseModifier == MATH_POS_INF && solidModifier == MATH_POS_INF) return 1.0f;
	return baseModifier == MATH_POS_INF ? solidModifier : baseModifier;
}

#define LIQUID_GRAVITY 0.02f
#define ROPE_GRAVITY   0.034f
void PhysicsComp_PhysicsTick(struct PhysicsComp* comp, Vector3 vel) {
	struct Entity* entity = comp->Entity;
	struct HacksComp* hacks = comp->Hacks;

	if (hacks->Noclip) entity->OnGround = false;
	Real32 baseSpeed = PhysicsComp_GetBaseSpeed(comp);
	Real32 verSpeed  = baseSpeed * (PhysicsComp_GetSpeed(hacks, 8.0f) / 5.0f);
	Real32 horSpeed  = baseSpeed * PhysicsComp_GetSpeed(hacks, 8.0f / 5.0f) * hacks->BaseHorSpeed;
	/* previously horSpeed used to be multiplied by factor of 0.02 in last case */
	/* it's now multiplied by 0.1, so need to divide by 5 so user speed modifier comes out same */

	/* TODO: this is a temp fix to avoid crashing for high horizontal speed */
	Math_Clamp(horSpeed, -75.0f, 75.0f);

	/* vertical speed never goes below: base speed * 1.0 */
	if (verSpeed < baseSpeed) verSpeed = baseSpeed;

	bool womSpeedBoost = hacks->CanDoubleJump && hacks->WOMStyleHacks;
	if (!hacks->Floating && womSpeedBoost) {
		if (comp->MultiJumps == 1)     { horSpeed *= 46.5f; verSpeed *= 7.5f; } 
		else if (comp->MultiJumps > 1) { horSpeed *= 93.0f; verSpeed *= 10.0f; }
	}

	if (Entity_TouchesAnyWater(entity) && !hacks->Floating) {
		Vector3 waterDrag = VECTOR3_CONST(0.8f, 0.8f, 0.8f);
		PhysicsComp_MoveNormal(comp, vel, 0.02f * horSpeed, waterDrag, LIQUID_GRAVITY, verSpeed);
	} else if (Entity_TouchesAnyLava(entity) && !hacks->Floating) {
		Vector3 lavaDrag = VECTOR3_CONST(0.5f, 0.5f, 0.5f);
		PhysicsComp_MoveNormal(comp, vel, 0.02f * horSpeed, lavaDrag, LIQUID_GRAVITY, verSpeed);
	} else if (Entity_TouchesAnyRope(entity) && !hacks->Floating) {
		Vector3 ropeDrag = VECTOR3_CONST(0.5f, 0.85f, 0.5f);
		PhysicsComp_MoveNormal(comp, vel, 0.02f * 1.7f, ropeDrag, ROPE_GRAVITY, verSpeed);
	} else {
		Real32 factor  = hacks->Floating || entity->OnGround ? 0.1f : 0.02f;
		Real32 gravity = comp->UseLiquidGravity ? LIQUID_GRAVITY : entity->Model->Gravity;

		if (hacks->Floating) {
			PhysicsComp_MoveFlying(comp, vel, factor * horSpeed, entity->Model->Drag, gravity, verSpeed);
		} else {
			PhysicsComp_MoveNormal(comp, vel, factor * horSpeed, entity->Model->Drag, gravity, verSpeed);
		}

		if (PhysicsComp_OnIce(entity) && !hacks->Floating) {
			/* limit components to +-0.25f by rescaling vector to [-0.25, 0.25] */
			if (Math_AbsF(entity->Velocity.X) > 0.25f || Math_AbsF(entity->Velocity.Z) > 0.25f) {
				Real32 xScale = Math_AbsF(0.25f / entity->Velocity.X);
				Real32 zScale = Math_AbsF(0.25f / entity->Velocity.Z);

				Real32 scale = min(xScale, zScale);
				entity->Velocity.X *= scale;
				entity->Velocity.Z *= scale;
			}
		} else if (entity->OnGround || hacks->Flying) {
			Vector3_Mul3By(&entity->Velocity, &entity->Model->GroundFriction); /* air drag or ground friction */
		}
	}

	if (entity->OnGround) comp->MultiJumps = 0;
}

static Real64 PhysicsComp_YPosAt(Int32 t, Real32 u) {
	/* v(t, u) = (4 + u) * (0.98^t) - 4, where u = initial velocity */
	/* x(t, u) = Σv(t, u) from 0 to t (since we work in discrete timesteps) */
	/* plugging into Wolfram Alpha gives 1 equation as */
	/* (0.98^t) * (-49u - 196) - 4t + 50u + 196 */
	Real64 a = Math_Exp(-0.0202027 * t); /* ~0.98^t */
	return a * (-49 * u - 196) - 4 * t + 50 * u + 196;
}

Real64 PhysicsComp_GetMaxHeight(Real32 u) {
	/* equation below comes from solving diff(x(t, u))= 0 */
	/* We only work in discrete timesteps, so test both rounded up and down */
	Real64 t = 49.49831645 * Math_Log(0.247483075 * u + 0.9899323);
	Real64 value_floor = PhysicsComp_YPosAt((Int32)t, u);
	Real64 value_ceil  = PhysicsComp_YPosAt((Int32)t + 1, u);
	return max(value_floor, value_ceil);
}

/* Calculates the jump velocity required such that when a client presses
the jump binding they will be able to jump up to the given height. */
void PhysicsComp_CalculateJumpVelocity(struct PhysicsComp* comp, Real32 jumpHeight) {
	comp->JumpVel = 0.0f;
	if (jumpHeight == 0.0f) return;

	if (jumpHeight >= 256.0f) comp->JumpVel = 10.0f;
	if (jumpHeight >= 512.0f) comp->JumpVel = 16.5f;
	if (jumpHeight >= 768.0f) comp->JumpVel = 22.5f;

	while (PhysicsComp_GetMaxHeight(comp->JumpVel) <= jumpHeight) { comp->JumpVel += 0.001f; }
}

void PhysicsComp_DoEntityPush(struct Entity* entity) {
	Int32 id;
	Vector3 dir; dir.Y = 0.0f;

	for (id = 0; id < ENTITIES_MAX_COUNT; id++) {
		struct Entity* other = Entities_List[id];
		if (!other || other == entity) continue;
		if (!other->Model->Pushes) continue;

		bool yIntersects =
			entity->Position.Y <= (other->Position.Y  + other->Size.Y) &&
			 other->Position.Y <= (entity->Position.Y + entity->Size.Y);
		if (!yIntersects) continue;

		dir.X = other->Position.X - entity->Position.X;
		dir.Z = other->Position.Z - entity->Position.Z;
		Real32 dist = dir.X	* dir.X + dir.Z * dir.Z;
		if (dist < 0.002f || dist > 1.0f) continue; /* TODO: range needs to be lower? */

		Vector3_Normalize(&dir, &dir);
		Real32 pushStrength = (1 - dist) / 32.0f; /* TODO: should be 24/25 */
		/* entity.Velocity -= dir * pushStrength */
		Vector3_Mul1By(&dir, pushStrength);
		Vector3_SubBy(&entity->Velocity, &dir);
	}
}


/*########################################################################################################################*
*----------------------------------------------------SoundsComponent------------------------------------------------------*
*#########################################################################################################################*/
Vector3 sounds_LastPos = { -1e25f, -1e25f, -1e25f };
bool sounds_AnyNonAir;
UInt8 sounds_Type;

static bool Sounds_CheckNonSolid(BlockID b) {
	UInt8 type = Block_StepSounds[b];
	UInt8 collide = Block_Collide[b];
	if (type != SOUND_NONE && collide != COLLIDE_SOLID) sounds_Type = type;

	if (Block_Draw[b] != DRAW_GAS) sounds_AnyNonAir = true;
	return false;
}

static bool Sounds_CheckSolid(BlockID b) {
	UInt8 type = Block_StepSounds[b];
	if (type != SOUND_NONE) sounds_Type = type;

	if (Block_Draw[b] != DRAW_GAS) sounds_AnyNonAir = true;
	return false;
}

static void SoundComp_GetSound(struct LocalPlayer* p) {
	Vector3 pos = p->Interp.Next.Pos;
	struct AABB bounds; Entity_GetBounds(&p->Base, &bounds);
	sounds_Type = SOUND_NONE;
	sounds_AnyNonAir = false;

	/* first check surrounding liquids/gas for sounds */
	Entity_TouchesAny(&bounds, Sounds_CheckNonSolid);
	if (sounds_Type != SOUND_NONE) return;

	/* then check block standing on */
	pos.Y -= 0.01f;
	Vector3I feetPos; Vector3I_Floor(&feetPos, &pos);
	BlockID blockUnder = World_SafeGetBlock_3I(feetPos);
	Real32 maxY = feetPos.Y + Block_MaxBB[blockUnder].Y;

	UInt8 typeUnder = Block_StepSounds[blockUnder];
	UInt8 collideUnder = Block_Collide[blockUnder];
	if (maxY >= pos.Y && collideUnder == COLLIDE_SOLID && typeUnder != SOUND_NONE) {
		sounds_AnyNonAir = true; sounds_Type = typeUnder; return;
	}

	/* then check all solid blocks at feet */
	bounds.Max.Y = bounds.Min.Y = pos.Y;
	Entity_TouchesAny(&bounds, Sounds_CheckSolid);
}

static bool SoundComp_DoPlaySound(struct LocalPlayer* p, Vector3 soundPos) {
	Vector3 delta; Vector3_Sub(&delta, &sounds_LastPos, &soundPos);
	Real32 distSq = Vector3_LengthSquared(&delta);
	bool enoughDist = distSq > 1.75f * 1.75f;
	/* just play every certain block interval when not animating */
	if (p->Base.Anim.Swing < 0.999f) return enoughDist;

	/* have our legs just crossed over the '0' point? */
	Real32 oldLegRot, newLegRot;
	if (Camera_Active->IsThirdPerson) {
		oldLegRot = Math_CosF(p->Base.Anim.WalkTimeO);
		newLegRot = Math_CosF(p->Base.Anim.WalkTimeN);
	} else {
		oldLegRot = Math_SinF(p->Base.Anim.WalkTimeO);
		newLegRot = Math_SinF(p->Base.Anim.WalkTimeN);
	}
	return Math_Sign(oldLegRot) != Math_Sign(newLegRot);
}

void SoundComp_Tick(bool wasOnGround) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	Vector3 soundPos = p->Interp.Next.Pos;
	SoundComp_GetSound(p);
	if (!sounds_AnyNonAir) soundPos = Vector3_BigPos();

	if (p->Base.OnGround && (SoundComp_DoPlaySound(p, soundPos) || !wasOnGround)) {
		Audio_PlayStepSound(sounds_Type);
		sounds_LastPos = soundPos;
	}
}
