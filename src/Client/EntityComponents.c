#include "EntityComponents.h"
#include "ExtMath.h"
#include "Events.h"
#include "Game.h"
#include "Player.h"
#include "Camera.h"
#include "Platform.h"

#define maxAngle (110 * MATH_DEG2RAD)
#define armMax (60.0f * MATH_DEG2RAD)
#define legMax (80.0f * MATH_DEG2RAD)
#define idleMax (3.0f * MATH_DEG2RAD)
#define idleXPeriod (2.0f * MATH_PI / 5.0f)
#define idleZPeriod (2.0f * MATH_PI / 3.5f)


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
	Real32 zRot = -idleZRot - verAngle * anim->Swing * maxAngle;
	Real32 horAngle = Math_Cos(anim->WalkTime) * anim->Swing * armMax * 1.5f;
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
	Real32 idleXRot = Math_Sin(idleTime * idleXPeriod) * idleMax;
	Real32 idleZRot = idleMax + Math_Cos(idleTime * idleZPeriod) * idleMax;

	anim->LeftArmX = (Math_Cos(anim->WalkTime) * anim->Swing * armMax) - idleXRot;
	anim->LeftArmZ = -idleZRot;
	anim->LeftLegX = -(Math_Cos(anim->WalkTime) * anim->Swing * legMax);
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

void HacksComponent_SetAll(HacksComponent* hacks, bool allowed) {
	hacks->CanAnyHacks = allowed; hacks->CanFly = allowed;
	hacks->CanNoclip = allowed; hacks->CanRespawn = allowed;
	hacks->CanSpeed = allowed; hacks->CanPushbackBlocks = allowed;
	hacks->CanUseThirdPersonCamera = allowed;
}

void HacksComponent_Init(HacksComponent* hacks) {
	Platform_MemSet(hacks, 0, sizeof(HacksComponent));
	HacksComponent_SetAll(hacks, true);
	hacks->SpeedMultiplier = 10.0f;
	hacks->Enabled = true;
	hacks->CanSeeAllNames = true;
	hacks->CanDoubleJump = true;
	hacks->MaxSpeedMultiplier = 1.0f;
	hacks->NoclipSlide = true;
	hacks->HacksFlags = String_FromRawBuffer(&hacks->HacksFlagsBuffer[0], 128);
}

bool HacksComponent_CanJumpHigher(HacksComponent* hacks) {
	return hacks->Enabled && hacks->CanAnyHacks && hacks->CanSpeed;
}

bool HacksComponent_Floating(HacksComponent* hacks) {
	return hacks->Noclip || hacks->Flying;
}

void HacksComponent_ParseHorizontalSpeed(HacksComponent* hacks) {
	String* joined = &hacks->HacksFlags;
	String horSpeed = String_FromConstant("horspeed=");
	Int32 start = String_IndexOfString(joined, &horSpeed);
	if (start < 0) return;
	start += horSpeed.length;

	Int32 end = String_IndexOf(joined, ' ', start);
	if (end < 0) end = joined->length;

	String speedStr = String_UNSAFE_Substring(joined, start, end - start);
	Real32 speed = 0.0f;
	if (!Convert_TryParseReal32(&speedStr, &speed) || speed <= 0.0f) return;
	hacks->MaxSpeedMultiplier = speed;
}

void HacksComponent_ParseFlag(HacksComponent* hacks, const UInt8* incFlag, const UInt8* excFlag, bool* target) {
	String include = String_FromReadonly(incFlag);
	String exclude = String_FromReadonly(excFlag);
	String* joined = &hacks->HacksFlags;

	if (String_ContainsString(joined, &include)) {
		*target = true;
	} else if (String_ContainsString(joined, &exclude)) {
		*target = false;
	}
}

void HacksComponent_ParseAllFlag(HacksComponent* hacks, const UInt8* incFlag, const UInt8* excFlag) {
	String include = String_FromReadonly(incFlag);
	String exclude = String_FromReadonly(excFlag);
	String* joined = &hacks->HacksFlags;

	if (String_ContainsString(joined, &include)) {
		HacksComponent_SetAll(hacks, true);
	} else if (String_ContainsString(joined, &exclude)) {
		HacksComponent_SetAll(hacks, false);
	}
}

/* Sets the user type of this user. This is used to control permissions for grass,
bedrock, water and lava blocks on servers that don't support CPE block permissions. */
void HacksComponent_SetUserType(HacksComponent* hacks, UInt8 value) {
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
void HacksComponent_CheckConsistency(HacksComponent* hacks) {
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
void HacksComponent_UpdateState(HacksComponent* hacks) {
	HacksComponent_SetAll(hacks, true);
	if (hacks->HacksFlags.length == 0) return;

	hacks->MaxSpeedMultiplier = 1;
	/* By default (this is also the case with WoM), we can use hacks. */
	String excHacks = String_FromConstant("-hax");
	if (String_ContainsString(&hacks->HacksFlags, &excHacks)) {
		HacksComponent_SetAll(hacks, false);
	}

	HacksComponent_ParseFlag(hacks, "+fly", "-fly", &hacks->CanFly);
	HacksComponent_ParseFlag(hacks, "+noclip", "-noclip", &hacks->CanNoclip);
	HacksComponent_ParseFlag(hacks, "+speed", "-speed", &hacks->CanSpeed);
	HacksComponent_ParseFlag(hacks, "+respawn", "-respawn", &hacks->CanRespawn);

	if (hacks->UserType == 0x64) {
		HacksComponent_ParseAllFlag(hacks, "+ophax", "-ophax");
	}
	HacksComponent_ParseHorizontalSpeed(hacks);

	HacksComponent_CheckConsistency(hacks);
	Event_RaiseVoid(&UserEvents_HackPermissionsChanged);
}