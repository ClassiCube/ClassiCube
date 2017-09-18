#include "Entity.h"
#include "ExtMath.h"
#include "World.h"
#include "Block.h"
#include "Events.h"
#include "Game.h"
#include "Player.h"
#include "Camera.h"
#include "Platform.h"
#include "Funcs.h"
#include "ModelCache.h"

Real32 LocationUpdate_Clamp(Real32 degrees) {
	degrees = Math_Mod(degrees, 360.0f);
	if (degrees < 0) degrees += 360.0f;
	return degrees;
}

void LocationUpdate_Construct(LocationUpdate* update, Real32 x, Real32 y, Real32 z,
	Real32 rotX, Real32 rotY, Real32 rotZ, Real32 headX, bool incPos, bool relPos) {
	update->Pos = Vector3_Create3(x, y, z);
	update->RotX = LocationUpdate_Clamp(rotX);
	update->RotY = LocationUpdate_Clamp(rotY);
	update->RotZ = LocationUpdate_Clamp(rotZ);
	update->HeadX = LocationUpdate_Clamp(headX);
	update->IncludesPosition = incPos;
	update->RelativePosition = relPos;
}

#define exc LOCATIONUPDATE_EXCLUDED
void LocationUpdate_Empty(LocationUpdate* update) {
	LocationUpdate_Construct(update, 0.0f, 0.0f, 0.0f, exc, exc, exc, exc, false, false);
}

void LocationUpdate_MakeOri(LocationUpdate* update, Real32 rotY, Real32 headX) {
	LocationUpdate_Construct(update, 0.0f, 0.0f, 0.0f, exc, rotY, exc, headX, false, false);
}

void LocationUpdate_MakePos(LocationUpdate* update, Vector3 pos, bool rel) {
	LocationUpdate_Construct(update, pos.X, pos.Y, pos.Z, exc, exc, exc, exc, true, rel);
}

void LocationUpdate_MakePosAndOri(LocationUpdate* update, Vector3 pos, Real32 rotY, Real32 headX, bool rel) {
	LocationUpdate_Construct(update, pos.X, pos.Y, pos.Z, exc, rotY, exc, headX, true, rel);
}


void Entity_Init(Entity* entity) {
	entity->ModelScale = Vector3_Create1(1.0f);
	entity->TextureId = -1;
	entity->MobTextureId = -1;
	entity->uScale = 1.0f;
	entity->vScale = 1.0f;
	UInt8* ptr = entity->ModelNameBuffer;
	entity->ModelName = String_FromRawBuffer(ptr, ENTITY_MAX_MODEL_LENGTH);
}

Vector3 Entity_GetEyePosition(Entity* entity) {
	Vector3 pos = entity->Position;
	pos.Y += entity->Model->GetEyeY(entity) * entity->ModelScale.Y;
	return pos;
}

void Entity_GetTransform(Entity* entity, Vector3 pos, Vector3 scale) {
	Matrix* m = &entity->Transform;
	*m = Matrix_Identity;
	Matrix tmp;

	Matrix_RotateZ(&tmp, -entity->RotZ * MATH_DEG2RAD);
	Matrix_MulBy(m, &tmp);
	Matrix_RotateX(&tmp, -entity->RotX * MATH_DEG2RAD);
	Matrix_MulBy(m, &tmp);
	Matrix_RotateY(&tmp, -entity->RotY * MATH_DEG2RAD);
	Matrix_MulBy(m, &tmp);
	Matrix_Scale(&tmp, scale.X, scale.Y, scale.Z);
	Matrix_MulBy(m, &tmp);
	Matrix_Translate(&tmp, pos.X, pos.Y, pos.Z);
	Matrix_MulBy(m, &tmp);
	/* return rotZ * rotX * rotY * scale * translate; */
}

void Entity_GetPickingBounds(Entity* entity, AABB* bb) {
	AABB_Offset(bb, &entity->ModelAABB, &entity->Position);
}

void Entity_GetBounds(Entity* entity, AABB* bb) {
	AABB_Make(bb, &entity->Position, &entity->Size);
}

void Entity_ParseScale(Entity* entity, String scale) {
	if (scale.buffer == NULL) return;
	Real32 value;
	if (!Convert_TryParseReal32(&scale, &value)) return;

	Real32 maxScale = entity->Model->MaxScale;
	Math_Clamp(value, 0.01f, maxScale);
	entity->ModelScale = Vector3_Create1(value);
}

void Entity_SetModel(Entity* entity, STRING_TRANSIENT String* model) {
	entity->ModelScale = Vector3_Create1(1.0f);
	entity->ModelBlock = BlockID_Air;
	String_Clear(&entity->ModelName);

	Int32 sep = String_IndexOf(model, '|', 0);
	String name, scale;
	if (sep == -1) {
		name  = *model;
		scale = String_MakeNull();	
	} else {
		name  = String_UNSAFE_SubstringAt(model, sep + 1);
		scale = String_UNSAFE_Substring(model, 0, sep);
	}

	/* 'giant' model kept for backwards compatibility */
	String giant = String_FromConstant("giant");
	if (String_CaselessEquals(model, &giant)) {		
		String_AppendConstant(&entity->ModelName, "humanoid");
		entity->ModelScale = Vector3_Create1(2.0f);
	} else if (Convert_TryParseUInt8(model, &entity->ModelBlock)) {
		String_AppendConstant(&entity->ModelName, "block");
	} else {
		String_AppendString(&entity->ModelName, &name);
	}

	entity->Model = ModelCache_Get(&entity->ModelName);
	Entity_ParseScale(entity, scale);
	entity->MobTextureId = -1;

	entity->Model->RecalcProperties(entity);
	Entity_UpdateModelBounds(entity);
}

void Entity_UpdateModelBounds(Entity* entity) {
	IModel* model = entity->Model;
	Vector3 baseSize = model->GetCollisionSize();
	Vector3_Multiply3(&entity->Size, &baseSize, &entity->ModelScale);

	AABB* bb = &entity->ModelAABB;
	model->GetPickingBounds(bb);
	Vector3_Multiply3(&bb->Min, &bb->Min, &entity->ModelScale);
	Vector3_Multiply3(&bb->Max, &bb->Max, &entity->ModelScale);
}

bool Entity_TouchesAny(AABB* bounds, TouchesAny_Condition condition) {
	Vector3I bbMin, bbMax;
	Vector3I_Floor(&bbMin, &bounds->Min);
	Vector3I_Floor(&bbMax, &bounds->Max);
	AABB blockBB;
	Vector3 v;

	/* Order loops so that we minimise cache misses */
	Int32 x, y, z;
	for (y = bbMin.Y; y <= bbMax.Y; y++) {
		v.Y = (Real32)y;
		for (z = bbMin.Z; z <= bbMax.Z; z++) {
			v.Z = (Real32)z;
			for (x = bbMin.X; x <= bbMax.X; x++) {
				if (!World_IsValidPos(x, y, z)) continue;
				v.X = (Real32)x;

				BlockID block = World_GetBlock(x, y, z);
				Vector3_Add(&blockBB.Min, &v, &Block_MinBB[block]);
				Vector3_Add(&blockBB.Max, &v, &Block_MaxBB[block]);

				if (!AABB_Intersects(&blockBB, bounds)) continue;
				if (condition(block)) return true;
			}
		}
	}
	return false;
}

bool Entity_IsRope(BlockID b) { return b == BlockID_Rope; }
bool Entity_TouchesAnyRope(Entity* entity) {
	AABB bounds; Entity_GetBounds(entity, &bounds);
	bounds.Max.Y += 0.5f / 16.0f;
	return Entity_TouchesAny(&bounds, Entity_IsRope);
}

Vector3 entity_liqExpand = { 0.25f / 16.0f, 0.0f / 16.0f, 0.25f / 16.0f };

bool Entity_IsLava(BlockID b) { return Block_ExtendedCollide[b] == CollideType_LiquidLava; }
bool Entity_TouchesAnyLava(Entity* entity) {
	AABB bounds; Entity_GetBounds(entity, &bounds);
	AABB_Offset(&bounds, &bounds, &entity_liqExpand);
	return Entity_TouchesAny(&bounds, Entity_IsLava);
}

bool Entity_IsWater(BlockID b) { return Block_ExtendedCollide[b] == CollideType_LiquidWater; }
bool Entity_TouchesAnyWater(Entity* entity) {
	AABB bounds; Entity_GetBounds(entity, &bounds);
	AABB_Offset(&bounds, &bounds, &entity_liqExpand);
	return Entity_TouchesAny(&bounds, Entity_IsWater);
}


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
	anim->WalkTime = 0.0f; anim->Swing = 0.0f; anim->BobStrength = 1.0f;
	anim->WalkTimeO = 0.0f; anim->SwingO = 0.0f; anim->BobStrengthO = 1.0f;
	anim->WalkTimeN = 0.0f; anim->SwingN = 0.0f; anim->BobStrengthN = 1.0f;

	anim->LeftLegX = 0.0f; anim->LeftLegZ = 0.0f; anim->RightLegX = 0.0f; anim->RightLegZ = 0.0f;
	anim->LeftArmX = 0.0f; anim->LeftArmZ = 0.0f; anim->RightArmX = 0.0f; anim->RightArmZ = 0.0f;
}

void AnimatedComp_Update(AnimatedComp* anim, Vector3 oldPos, Vector3 newPos, Real64 delta, bool onGround) {
	anim->WalkTimeO = anim->WalkTimeN;
	anim->SwingO = anim->SwingN;
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

	anim->BobbingHor = Math_Cos(anim->WalkTime)              * anim->Swing * (2.5f / 16.0f);
	anim->BobbingVer = Math_AbsF(Math_Sin(anim->WalkTime))   * anim->Swing * (2.5f / 16.0f);
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
	hacks->MaxSpeedMultiplier = 1.0f;
	hacks->MaxJumps = 1;
	hacks->NoclipSlide = true;
	hacks->CanBePushed = true;
	hacks->HacksFlags = String_FromRawBuffer(&hacks->HacksFlagsBuffer[0], 128);
}

bool HacksComp_CanJumpHigher(HacksComp* hacks) {
	return hacks->Enabled && hacks->CanAnyHacks && hacks->CanSpeed;
}

bool HacksComp_Floating(HacksComp* hacks) {
	return hacks->Noclip || hacks->Flying;
}

String HacksComp_GetFlagValue(String* flag, HacksComp* hacks) {
	String* joined = &hacks->HacksFlags;
	Int32 start = String_IndexOfString(joined, flag);
	if (start < 0) return String_MakeNull();
	start += flag->length;

	Int32 end = String_IndexOf(joined, ' ', start);
	if (end < 0) end = joined->length;

	return String_UNSAFE_Substring(joined, start, end - start);
}

void HacksComp_ParseHorizontalSpeed(HacksComp* hacks) {
	String horSpeedFlag = String_FromConstant("horspeed=");
	String speedStr = HacksComp_GetFlagValue(&horSpeedFlag, hacks);
	if (speedStr.length == 0) return;

	Real32 speed = 0.0f;
	if (!Convert_TryParseReal32(&speedStr, &speed) || speed <= 0.0f) return;
	hacks->MaxSpeedMultiplier = speed;
}

void HacksComp_ParseMultiSpeed(HacksComp* hacks) {
	String jumpsFlag = String_FromConstant("jumps=");
	String jumpsStr = HacksComp_GetFlagValue(&jumpsFlag, hacks);
	if (jumpsStr.length == 0) return;

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

/* Sets the user type of this user. This is used to control permissions for grass,
bedrock, water and lava blocks on servers that don't support CPE block permissions. */
void HacksComp_SetUserType(HacksComp* hacks, UInt8 value) {
	bool isOp = value >= 100 && value <= 127;
	hacks->UserType = value;
	Block_CanPlace[BlockID_Bedrock] = isOp;
	Block_CanDelete[BlockID_Bedrock] = isOp;

	Block_CanPlace[BlockID_Water] = isOp;
	Block_CanPlace[BlockID_StillWater] = isOp;
	Block_CanPlace[BlockID_Lava] = isOp;
	Block_CanPlace[BlockID_StillLava] = isOp;
	hacks->CanSeeAllNames = isOp;
}

/* Disables any hacks if their respective CanHackX value is set to false. */
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


/* Updates ability to use hacks, and raises HackPermissionsChanged event.
Parses hack flags specified in the motd and/or name of the server.
Recognises +/-hax, +/-fly, +/-noclip, +/-speed, +/-respawn, +/-ophax, and horspeed=xyz */
void HacksComp_UpdateState(HacksComp* hacks) {
	HacksComp_SetAll(hacks, true);
	if (hacks->HacksFlags.length == 0) return;

	hacks->MaxSpeedMultiplier = 1;
	hacks->MaxJumps = 1;
	hacks->CanBePushed = true;

	/* By default (this is also the case with WoM), we can use hacks. */
	String excHacks = String_FromConstant("-hax");
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
	if (next == LOCATIONUPDATE_EXCLUDED) return cur;
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

	cur->RotX  = NetInterpComp_Next(update->RotX,  cur->RotX);
	cur->RotZ  = NetInterpComp_Next(update->RotZ,  cur->RotZ);
	cur->HeadX = NetInterpComp_Next(update->HeadX, cur->HeadX);
	cur->HeadY = NetInterpComp_Next(update->RotY,  cur->HeadY);

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
	if (next == LOCATIONUPDATE_EXCLUDED) return cur;
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

	next->RotX  = LocalInterpComp_Next(update->RotX,  next->RotX,  &prev->RotX,  interpolate);
	next->RotZ  = LocalInterpComp_Next(update->RotZ,  next->RotZ,  &prev->RotZ,  interpolate);
	next->HeadX = LocalInterpComp_Next(update->HeadX, next->HeadX, &prev->HeadX, interpolate);
	next->HeadY = LocalInterpComp_Next(update->RotY,  next->HeadY, &prev->HeadY, interpolate);

	if (update->RotY != LOCATIONUPDATE_EXCLUDED) {
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