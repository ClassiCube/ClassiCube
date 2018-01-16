#include "EntityComponents.h"
#include "ExtMath.h"
#include "World.h"
#include "Block.h"
#include "Event.h"
#include "Game.h"
#include "Player.h"
#include "Platform.h"
#include "Camera.h"
#include "Funcs.h"
#include "VertexStructs.h"
#include "GraphicsAPI.h"
#include "GraphicsCommon.h"
#include "ModelCache.h"

#define ANIM_MAX_ANGLE (110 * MATH_DEG2RAD)
#define ANIM_ARM_MAX (60.0f * MATH_DEG2RAD)
#define ANIM_LEG_MAX (80.0f * MATH_DEG2RAD)
#define ANIM_IDLE_MAX (3.0f * MATH_DEG2RAD)
#define ANIM_IDLE_XPERIOD (2.0f * MATH_PI / 5.0f)
#define ANIM_IDLE_ZPERIOD (2.0f * MATH_PI / 3.5f)

void AnimatedComp_DoTilt(Real32* tilt, bool reduce) {
	if (reduce) {
		(*tilt) *= 0.84f;
	} else {
		(*tilt) += 0.1f;
	}
	Math_Clamp(*tilt, 0.0f, 1.0f);
}

void AnimatedComp_PerpendicularAnim(AnimatedComp* anim, Real32 flapSpeed, Real32 idleXRot, Real32 idleZRot, bool left) {
	Real32 verAngle = 0.5f + 0.5f * Math_Sin(anim->WalkTime * flapSpeed);
	Real32 zRot = -idleZRot - verAngle * anim->Swing * ANIM_MAX_ANGLE;
	Real32 horAngle = Math_Cos(anim->WalkTime) * anim->Swing * ANIM_ARM_MAX * 1.5f;
	Real32 xRot = idleXRot + horAngle;

	if (left) {
		anim->LeftArmX = xRot;  anim->LeftArmZ = zRot;
	} else {
		anim->RightArmX = xRot; anim->RightArmZ = zRot;
	}
}

void AnimatedComp_CalcHumanAnim(AnimatedComp* anim, Real32 idleXRot, Real32 idleZRot) {
	AnimatedComp_PerpendicularAnim(anim, 0.23f, idleXRot, idleZRot, true);
	AnimatedComp_PerpendicularAnim(anim, 0.28f, idleXRot, idleZRot, false);
	anim->RightArmX = -anim->RightArmX; anim->RightArmZ = -anim->RightArmZ;
}

void AnimatedComp_Init(AnimatedComp* anim) {
	anim->BobbingHor = 0.0f; anim->BobbingVer = 0.0f; anim->BobbingModel = 0.0f;
	anim->WalkTime   = 0.0f; anim->Swing      = 0.0f; anim->BobStrength  = 1.0f;
	anim->WalkTimeO  = 0.0f; anim->SwingO     = 0.0f; anim->BobStrengthO = 1.0f;
	anim->WalkTimeN  = 0.0f; anim->SwingN     = 0.0f; anim->BobStrengthN = 1.0f;

	anim->LeftLegX = 0.0f; anim->LeftLegZ = 0.0f; anim->RightLegX = 0.0f; anim->RightLegZ = 0.0f;
	anim->LeftArmX = 0.0f; anim->LeftArmZ = 0.0f; anim->RightArmX = 0.0f; anim->RightArmZ = 0.0f;
}

void AnimatedComp_Update(AnimatedComp* anim, Vector3 oldPos, Vector3 newPos, Real64 delta, bool onGround) {
	anim->WalkTimeO = anim->WalkTimeN;
	anim->SwingO    = anim->SwingN;
	Real32 dx = newPos.X - oldPos.X;
	Real32 dz = newPos.Z - oldPos.Z;
	Real32 distance = Math_Sqrt(dx * dx + dz * dz);

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
		AnimatedComp_DoTilt(&anim->BobStrengthN, !Game_ViewBobbing || !onGround);
	}
}


void AnimatedComp_GetCurrent(AnimatedComp* anim, Real32 t, bool calcHumanAnims) {
	anim->Swing = Math_Lerp(anim->SwingO, anim->SwingN, t);
	anim->WalkTime = Math_Lerp(anim->WalkTimeO, anim->WalkTimeN, t);
	anim->BobStrength = Math_Lerp(anim->BobStrengthO, anim->BobStrengthN, t);

	Real32 idleTime = (Real32)Game_Accumulator;
	Real32 idleXRot = Math_Sin(idleTime * ANIM_IDLE_XPERIOD) * ANIM_IDLE_MAX;
	Real32 idleZRot = ANIM_IDLE_MAX + Math_Cos(idleTime * ANIM_IDLE_ZPERIOD) * ANIM_IDLE_MAX;

	anim->LeftArmX = (Math_Cos(anim->WalkTime) * anim->Swing * ANIM_ARM_MAX) - idleXRot;
	anim->LeftArmZ = -idleZRot;
	anim->LeftLegX = -(Math_Cos(anim->WalkTime) * anim->Swing * ANIM_LEG_MAX);
	anim->LeftLegZ = 0;

	anim->RightLegX = -anim->LeftLegX; anim->RightLegZ = -anim->LeftLegZ;
	anim->RightArmX = -anim->LeftArmX; anim->RightArmZ = -anim->LeftArmZ;

	anim->BobbingHor   = Math_Cos(anim->WalkTime)            * anim->Swing * (2.5f / 16.0f);
	anim->BobbingVer   = Math_AbsF(Math_Sin(anim->WalkTime)) * anim->Swing * (2.5f / 16.0f);
	anim->BobbingModel = Math_AbsF(Math_Cos(anim->WalkTime)) * anim->Swing * (4.0f / 16.0f);

	if (calcHumanAnims && !Game_SimpleArmsAnim) {
		AnimatedComp_CalcHumanAnim(anim, idleXRot, idleZRot);
	}
}


void TiltComp_Init(TiltComp* anim) {
	anim->TiltX = 0.0f; anim->TiltY = 0.0f; anim->VelTiltStrength = 1.0f;
	anim->VelTiltStrengthO = 1.0f; anim->VelTiltStrengthN = 1.0f;
}

void TiltComp_Update(TiltComp* anim, Real64 delta) {
	anim->VelTiltStrengthO = anim->VelTiltStrengthN;
	LocalPlayer* p = &LocalPlayer_Instance;

	/* TODO: the Tilt code was designed for 60 ticks/second, fix it up for 20 ticks/second */
	Int32 i;
	for (i = 0; i < 3; i++) {
		AnimatedComp_DoTilt(&anim->VelTiltStrengthN, p->Hacks.Noclip || p->Hacks.Flying);
	}
}

void TiltComp_GetCurrent(TiltComp* anim, Real32 t) {
	LocalPlayer* p = &LocalPlayer_Instance;
	anim->VelTiltStrength = Math_Lerp(anim->VelTiltStrengthO, anim->VelTiltStrengthN, t);

	AnimatedComp* pAnim = &p->Base.Base.Anim;
	anim->TiltX = Math_Cos(pAnim->WalkTime) * pAnim->Swing * (0.15f * MATH_DEG2RAD);
	anim->TiltY = Math_Sin(pAnim->WalkTime) * pAnim->Swing * (0.15f * MATH_DEG2RAD);
}


void HacksComp_SetAll(HacksComp* hacks, bool allowed) {
	hacks->CanAnyHacks = allowed; hacks->CanFly = allowed;
	hacks->CanNoclip = allowed; hacks->CanRespawn = allowed;
	hacks->CanSpeed = allowed; hacks->CanPushbackBlocks = allowed;
	hacks->CanUseThirdPersonCamera = allowed;
}

void HacksComp_Init(HacksComp* hacks) {
	Platform_MemSet(hacks, 0, sizeof(HacksComp));
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

bool HacksComp_CanJumpHigher(HacksComp* hacks) {
	return hacks->Enabled && hacks->CanAnyHacks && hacks->CanSpeed;
}

bool HacksComp_Floating(HacksComp* hacks) {
	return hacks->Noclip || hacks->Flying;
}

String HacksComp_UNSAFE_FlagValue(String* flag, HacksComp* hacks) {
	String* joined = &hacks->HacksFlags;
	Int32 start = String_IndexOfString(joined, flag);
	if (start < 0) return String_MakeNull();
	start += flag->length;

	Int32 end = String_IndexOf(joined, ' ', start);
	if (end < 0) end = joined->length;

	return String_UNSAFE_Substring(joined, start, end - start);
}

void HacksComp_ParseHorizontalSpeed(HacksComp* hacks) {
	String horSpeedFlag = String_FromConst("horspeed=");
	String speedStr = HacksComp_UNSAFE_FlagValue(&horSpeedFlag, hacks);
	if (speedStr.length == 0) return;

	Real32 speed = 0.0f;
	if (!Convert_TryParseReal32(&speedStr, &speed) || speed <= 0.0f) return;
	hacks->BaseHorSpeed = speed;
}

void HacksComp_ParseMultiSpeed(HacksComp* hacks) {
	String jumpsFlag = String_FromConst("jumps=");
	String jumpsStr = HacksComp_UNSAFE_FlagValue(&jumpsFlag, hacks);
	if (jumpsStr.length == 0 || Game_ClassicMode) return;

	Int32 jumps = 0;
	if (!Convert_TryParseInt32(&jumpsStr, &jumps) || jumps < 0) return;
	hacks->MaxJumps = jumps;
}

void HacksComp_ParseFlag(HacksComp* hacks, const UInt8* incFlag, const UInt8* excFlag, bool* target) {
	String include = String_FromReadonly(incFlag);
	String exclude = String_FromReadonly(excFlag);
	String* joined = &hacks->HacksFlags;

	if (String_ContainsString(joined, &include)) {
		*target = true;
	} else if (String_ContainsString(joined, &exclude)) {
		*target = false;
	}
}

void HacksComp_ParseAllFlag(HacksComp* hacks, const UInt8* incFlag, const UInt8* excFlag) {
	String include = String_FromReadonly(incFlag);
	String exclude = String_FromReadonly(excFlag);
	String* joined = &hacks->HacksFlags;

	if (String_ContainsString(joined, &include)) {
		HacksComp_SetAll(hacks, true);
	} else if (String_ContainsString(joined, &exclude)) {
		HacksComp_SetAll(hacks, false);
	}
}

void HacksComp_SetUserType(HacksComp* hacks, UInt8 value) {
	bool isOp = value >= 100 && value <= 127;
	hacks->UserType = value;
	hacks->CanSeeAllNames = isOp;

	Block_CanPlace[BLOCK_BEDROCK] = isOp;
	Block_CanDelete[BLOCK_BEDROCK] = isOp;
	Block_CanPlace[BLOCK_WATER] = isOp;
	Block_CanPlace[BLOCK_STILL_WATER] = isOp;
	Block_CanPlace[BLOCK_LAVA] = isOp;
	Block_CanPlace[BLOCK_STILL_LAVA] = isOp;
}

void HacksComp_CheckConsistency(HacksComp* hacks) {
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

void HacksComp_UpdateState(HacksComp* hacks) {
	HacksComp_SetAll(hacks, true);
	if (hacks->HacksFlags.length == 0) return;

	hacks->BaseHorSpeed = 1;
	hacks->MaxJumps = 1;
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
	HacksComp_ParseHorizontalSpeed(hacks);
	HacksComp_ParseMultiSpeed(hacks);

	HacksComp_CheckConsistency(hacks);
	Event_RaiseVoid(&UserEvents_HackPermissionsChanged);
}


void InterpComp_RemoveOldestRotY(InterpComp* interp) {
	Int32 i;
	for (i = 0; i < Array_NumElements(interp->RotYStates); i++) {
		interp->RotYStates[i] = interp->RotYStates[i + 1];
	}
	interp->RotYCount--;
}

void InterpComp_AddRotY(InterpComp* interp, Real32 state) {
	if (interp->RotYCount == Array_NumElements(interp->RotYStates)) {
		InterpComp_RemoveOldestRotY(interp);
	}
	interp->RotYStates[interp->RotYCount] = state; interp->RotYCount++;
}

void InterpComp_AdvanceRotY(InterpComp* interp) {
	interp->PrevRotY = interp->NextRotY;
	if (interp->RotYCount == 0) return;

	interp->NextRotY = interp->RotYStates[0];
	InterpComp_RemoveOldestRotY(interp);
}

void InterpComp_LerpAngles(InterpComp* interp, Entity* entity, Real32 t) {
	InterpState* prev = &interp->Prev;
	InterpState* next = &interp->Next;
	entity->HeadX = Math_LerpAngle(prev->HeadX, next->HeadX, t);
	entity->HeadY = Math_LerpAngle(prev->HeadY, next->HeadY, t);
	entity->RotX = Math_LerpAngle(prev->RotX, next->RotX, t);
	entity->RotY = Math_LerpAngle(interp->PrevRotY, interp->NextRotY, t);
	entity->RotZ = Math_LerpAngle(prev->RotZ, next->RotZ, t);
}

void InterpComp_SetPos(InterpState* state, LocationUpdate* update) {
	if (update->RelativePosition) {
		Vector3_Add(&state->Pos, &state->Pos, &update->Pos);
	} else {
		state->Pos = update->Pos;
	}
}

Real32 NetInterpComp_Next(Real32 next, Real32 cur) {
	if (next == MATH_POS_INF) return cur;
	return next;
}

void NetInterpComp_RemoveOldestState(NetInterpComp* interp) {
	Int32 i;
	for (i = 0; i < Array_NumElements(interp->States); i++) {
		interp->States[i] = interp->States[i + 1];
	}
	interp->StatesCount--;
}

void NetInterpComp_AddState(NetInterpComp* interp, InterpState state) {
	if (interp->StatesCount == Array_NumElements(interp->States)) {
		NetInterpComp_RemoveOldestState(interp);
	}
	interp->States[interp->StatesCount] = state; interp->StatesCount++;
}

void NetInterpComp_SetLocation(NetInterpComp* interp, LocationUpdate* update, bool interpolate) {
	InterpState last = interp->Cur;
	InterpState* cur = &interp->Cur;
	if (update->IncludesPosition) {
		InterpComp_SetPos(cur, update);
	}

	cur->RotX = NetInterpComp_Next(update->RotX, cur->RotX);
	cur->RotZ = NetInterpComp_Next(update->RotZ, cur->RotZ);
	cur->HeadX = NetInterpComp_Next(update->HeadX, cur->HeadX);
	cur->HeadY = NetInterpComp_Next(update->RotY, cur->HeadY);

	if (!interpolate) {
		interp->Base.Prev = *cur; interp->Base.PrevRotY = cur->HeadY;
		interp->Base.Next = *cur; interp->Base.NextRotY = cur->HeadY;
		interp->Base.RotYCount = 0; interp->StatesCount = 0;
	} else {
		/* Smoother interpolation by also adding midpoint. */
		InterpState mid;
		Vector3_Lerp(&mid.Pos, &last.Pos, &cur->Pos, 0.5f);
		mid.RotX = Math_LerpAngle(last.RotX, cur->RotX, 0.5f);
		mid.RotZ = Math_LerpAngle(last.RotZ, cur->RotZ, 0.5f);
		mid.HeadX = Math_LerpAngle(last.HeadX, cur->HeadX, 0.5f);
		mid.HeadY = Math_LerpAngle(last.HeadY, cur->HeadY, 0.5f);
		NetInterpComp_AddState(interp, mid);
		NetInterpComp_AddState(interp, *cur);

		/* Head rotation lags behind body a tiny bit */
		InterpComp_AddRotY(&interp->Base, Math_LerpAngle(last.HeadY, cur->HeadY, 0.33333333f));
		InterpComp_AddRotY(&interp->Base, Math_LerpAngle(last.HeadY, cur->HeadY, 0.66666667f));
		InterpComp_AddRotY(&interp->Base, Math_LerpAngle(last.HeadY, cur->HeadY, 1.00000000f));
	}
}

void NetInterpComp_AdvanceState(NetInterpComp* interp) {
	interp->Base.Prev = interp->Base.Next;
	if (interp->StatesCount > 0) {
		interp->Base.Next = interp->States[0];
		NetInterpComp_RemoveOldestState(interp);
	}
	InterpComp_AdvanceRotY(&interp->Base);
}

Real32 LocalInterpComp_Next(Real32 next, Real32 cur, Real32* last, bool interpolate) {
	if (next == MATH_POS_INF) return cur;
	if (!interpolate) *last = next;
	return next;
}

void LocalInterpComp_SetLocation(InterpComp* interp, LocationUpdate* update, bool interpolate) {
	Entity* entity = &LocalPlayer_Instance.Base.Base;
	InterpState* prev = &interp->Prev;
	InterpState* next = &interp->Next;

	if (update->IncludesPosition) {
		InterpComp_SetPos(next, update);
		Real32 blockOffset = next->Pos.Y - Math_Floor(next->Pos.Y);
		if (blockOffset < ENTITY_ADJUSTMENT) {
			next->Pos.Y += ENTITY_ADJUSTMENT;
		}
		if (!interpolate) {
			prev->Pos = next->Pos;
			entity->Position = next->Pos;
		}
	}

	next->RotX = LocalInterpComp_Next(update->RotX, next->RotX, &prev->RotX, interpolate);
	next->RotZ = LocalInterpComp_Next(update->RotZ, next->RotZ, &prev->RotZ, interpolate);
	next->HeadX = LocalInterpComp_Next(update->HeadX, next->HeadX, &prev->HeadX, interpolate);
	next->HeadY = LocalInterpComp_Next(update->RotY, next->HeadY, &prev->HeadY, interpolate);

	if (update->RotY != MATH_POS_INF) {
		if (!interpolate) {
			interp->NextRotY = update->RotY;
			entity->RotY = update->RotY;
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

void LocalInterpComp_AdvanceState(InterpComp* interp) {
	Entity* entity = &LocalPlayer_Instance.Base.Base;
	interp->Prev = interp->Next;
	entity->Position = interp->Next.Pos;
	InterpComp_AdvanceRotY(interp);
}


Real32 ShadowComponent_radius = 7.0f;
typedef struct ShadowData_ { Real32 Y; BlockID Block; UInt8 A; } ShadowData;

bool lequal(Real32 a, Real32 b) { return a < b || Math_AbsF(a - b) < 0.001f; }
void ShadowComponent_DrawCoords(VertexP3fT2fC4b** vertices, Entity* entity, ShadowData* data, Real32 x1, Real32 z1, Real32 x2, Real32 z2) {
	Vector3 cen = entity->Position;
	if (lequal(x2, x1) || lequal(z2, z1)) return;
	Real32 radius = ShadowComponent_radius;

	Real32 u1 = (x1 - cen.X) * 16.0f / (radius * 2) + 0.5f;
	Real32 v1 = (z1 - cen.Z) * 16.0f / (radius * 2) + 0.5f;
	Real32 u2 = (x2 - cen.X) * 16.0f / (radius * 2) + 0.5f;
	Real32 v2 = (z2 - cen.Z) * 16.0f / (radius * 2) + 0.5f;
	if (u2 <= 0.0f || v2 <= 0.0f || u1 >= 1.0f || v1 >= 1.0f) return;

	radius /= 16.0f;
	x1 = max(x1, cen.X - radius); u1 = max(u1, 0.0f);
	z1 = max(z1, cen.Z - radius); v1 = max(v1, 0.0f);
	x2 = min(x2, cen.X + radius); u2 = min(u2, 1.0f);
	z2 = min(z2, cen.Z + radius); v2 = min(v2, 1.0f);

	PackedCol col = PACKEDCOL_CONST(255, 255, 255, data->A);
	VertexP3fT2fC4b* ptr = *vertices;
	VertexP3fT2fC4b v; v.Y = data->Y; v.Col = col;

	v.X = x1; v.Z = z1; v.U = u1; v.V = v1; *ptr = v; ptr++;
	v.X = x2;           v.U = u2;           *ptr = v; ptr++;
	          v.Z = z2;           v.V = v2; *ptr = v; ptr++;
	v.X = x1;           v.U = u1;           *ptr = v; ptr++;

	*vertices = ptr;
}

void ShadowComponent_DrawSquareShadow(VertexP3fT2fC4b** vertices, Real32 y, Real32 x, Real32 z) {
	PackedCol col = PACKEDCOL_CONST(255, 255, 255, 220);
	TextureRec rec = TextureRec_FromRegion(63.0f / 128.0f, 63.0f / 128.0f, 1.0f / 128.0f, 1.0f / 128.0f);
	VertexP3fT2fC4b* ptr = *vertices;
	VertexP3fT2fC4b v; v.Y = y; v.Col = col;

	v.X = x;        v.Z = z;        v.U = rec.U1; v.V = rec.V1; *ptr = v; ptr++;
	v.X = x + 1.0f;                 v.U = rec.U2;               *ptr = v; ptr++;
	                v.Z = z + 1.0f;               v.V = rec.V2; *ptr = v; ptr++;
	v.X = x;                        v.U = rec.U1;               *ptr = v; ptr++;

	*vertices = ptr;
}

void ShadowComponent_DrawCircle(VertexP3fT2fC4b** vertices, Entity* entity, ShadowData* data, Real32 x, Real32 z) {
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

BlockID ShadowComponent_GetBlock(Int32 x, Int32 y, Int32 z) {
	if (x < 0 || z < 0 || x >= World_Width || z >= World_Length) {
		if (y == WorldEnv_EdgeHeight - 1)
			return Block_Draw[WorldEnv_EdgeBlock] == DRAW_GAS ? BLOCK_AIR : BLOCK_BEDROCK;
		if (y == WorldEnv_SidesHeight - 1)
			return Block_Draw[WorldEnv_SidesBlock] == DRAW_GAS ? BLOCK_AIR : BLOCK_BEDROCK;
		return BLOCK_AIR;
	}
	return World_GetBlock(x, y, z);
}

void ShadowComponent_CalcAlpha(Real32 playerY, ShadowData* data) {
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

bool ShadowComponent_GetBlocks(Entity* entity, Vector3I* coords, Real32 x, Real32 z, Int32 posY, ShadowData* data) {
	Int32 blockX = Math_Floor(x), blockZ = Math_Floor(z);
	Vector3I p = Vector3I_Create3(blockX, 0, blockZ);	

	/* Check we have not processed this particular block already */
	UInt32 i, posCount = 0;
	ShadowData zeroData = { 0.0f, 0, 0 };
	for (i = 0; i < 4; i++) {
		if (Vector3I_Equals(&coords[i], &p)) return false;
		if (coords[i].X != Int32_MinValue) posCount++;
		data[i] = zeroData;
	}
	coords[posCount] = p;

	UInt32 count = 0;
	ShadowData* cur = data;
	Vector3 Position = entity->Position;

	while (posY >= 0 && count < 4) {
		BlockID block = ShadowComponent_GetBlock(blockX, posY, blockZ);
		posY--;

		UInt8 draw = Block_Draw[block];
		if (draw == DRAW_GAS || draw == DRAW_SPRITE || Block_IsLiquid[block]) continue;
		Real32 blockY = posY + 1.0f + Block_MaxBB[block].Y;
		if (blockY >= Position.Y + 0.01f) continue;

		cur->Block = block; cur->Y = blockY;
		ShadowComponent_CalcAlpha(Position.Y, cur);
		count++; cur++;

		/* Check if the casted shadow will continue on further down. */
		if (Block_MinBB[block].X == 0.0f && Block_MaxBB[block].X == 1.0f &&
			Block_MinBB[block].Z == 0.0f && Block_MaxBB[block].Z == 1.0f) return true;
	}

	if (count < 4) {
		cur->Block = WorldEnv_EdgeBlock; cur->Y = 0.0f;
		ShadowComponent_CalcAlpha(Position.Y, cur);
		count++; cur++;
	}
	return true;
}

#define size 128
#define half (size / 2)
void ShadowComponent_MakeTex(void) {
	UInt8 pixels[Bitmap_DataSize(size, size)];
	Bitmap bmp; Bitmap_Create(&bmp, size, size, pixels);

	UInt32 inPix = PackedCol_ARGB(0, 0, 0, 200);
	UInt32 outPix = PackedCol_ARGB(0, 0, 0, 0);

	UInt32 x, y;
	for (y = 0; y < size; y++) {
		UInt32* row = Bitmap_GetRow(&bmp, y);
		for (x = 0; x < size; x++) {
			Real64 dist = 
				(half - (x + 0.5)) * (half - (x + 0.5)) +
				(half - (y + 0.5)) * (half - (y + 0.5));
			row[x] = dist < half * half ? inPix : outPix;
		}
	}
	ShadowComponent_ShadowTex = Gfx_CreateTexture(&bmp, false, false);
}

void ShadowComponent_Draw(Entity* entity) {
	Vector3 Position = entity->Position;
	if (Position.Y < 0.0f) return;

	Real32 posX = Position.X, posZ = Position.Z;
	Int32 posY = min((Int32)Position.Y, World_MaxY);
	GfxResourceID vb;
	ShadowComponent_radius = 7.0f * min(entity->ModelScale.Y, 1.0f) * entity->Model->ShadowScale;

	VertexP3fT2fC4b vertices[128];
	Vector3I coords[4];
	ShadowData data[4];
	for (Int32 i = 0; i < 4; i++) {
		coords[i] = Vector3I_Create1(Int32_MinValue);
	}

	/* TODO: Should shadow component use its own VB? */
	VertexP3fT2fC4b* ptr = vertices;
	if (Entities_ShadowMode == SHADOW_MODE_SNAP_TO_BLOCK) {
		vb = GfxCommon_texVb;
		if (!ShadowComponent_GetBlocks(entity, coords, posX, posZ, posY, data)) return;

		Real32 x1 = (Real32)Math_Floor(posX), z1 = (Real32)Math_Floor(posZ);
		ShadowComponent_DrawSquareShadow(&ptr, data[0].Y, x1, z1);
	} else {
		vb = ModelCache_Vb;
		Real32 radius = ShadowComponent_radius / 16.0f;

		Real32 x = posX - radius, z = posZ - radius;
		if (ShadowComponent_GetBlocks(entity, coords, x, z, posY, data) && data[0].A > 0) {
			ShadowComponent_DrawCircle(&ptr, entity, data, x, z);
		}

		x = max(posX - radius, Math_Floor(posX + radius));
		if (ShadowComponent_GetBlocks(entity, coords, x, z, posY, data) && data[0].A > 0) {
			ShadowComponent_DrawCircle(&ptr, entity, data, x, z);
		}

		z = max(posZ - radius, Math_Floor(posZ + radius));
		if (ShadowComponent_GetBlocks(entity, coords, x, z, posY, data) && data[0].A > 0) {
			ShadowComponent_DrawCircle(&ptr, entity, data, x, z);
		}

		x = posX - radius;
		if (ShadowComponent_GetBlocks(entity, coords, x, z, posY, data) && data[0].A > 0) {
			ShadowComponent_DrawCircle(&ptr, entity, data, x, z);
		}
	}

	if (ptr == vertices) return;
	if (ShadowComponent_ShadowTex == NULL) {
		ShadowComponent_MakeTex();
	}

	if (!ShadowComponent_BoundShadowTex) {
		Gfx_BindTexture(ShadowComponent_ShadowTex);
		ShadowComponent_BoundShadowTex = true;
	}

	UInt32 vCount = (UInt32)(ptr - vertices) / VertexP3fT2fC4b_Size;
	GfxCommon_UpdateDynamicVb_IndexedTris(vb, vertices, vCount);
}
