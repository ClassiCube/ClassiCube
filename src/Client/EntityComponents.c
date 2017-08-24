#include "EntityComponents.h"
#include "ExtMath.h"
#include "Game.h"
#include "Player.h"

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

/* Entity component that performs management of hack states. */
typedef struct HacksComponent_ {
	UInt8 UserType;
	/* Speed player move at, relative to normal speed, when the 'speeding' key binding is held down. */
	Real32 SpeedMultiplier = 10;
	/* Whether blocks that the player places that intersect themselves, should cause the player to
	be pushed back in the opposite direction of the placed block. */
	bool PushbackPlacing;
	/* Whether the player should be able to step up whole blocks, instead of just slabs. */
	bool FullBlockStep;

	/* Whether the player has allowed hacks usage as an option. Note 'can use X' set by the server override this. */
	bool Enabled = true;
	/* Whether the player is allowed to use any type of hacks. */
	bool CanAnyHacks = true;
	/* Whether the player is allowed to use the types of cameras that use third person. */
	bool CanUseThirdPersonCamera = true;
	/* Whether the player is allowed to increase its speed beyond the normal walking speed. */
	bool CanSpeed = true;
	/* Whether the player is allowed to fly in the world. */
	bool CanFly = true;
	/* Whether the player is allowed to teleport to their respawn coordinates. */
	bool CanRespawn = true;
	/* Whether the player is allowed to pass through all blocks. */
	bool CanNoclip = true;
	/* Whether the player is allowed to use pushback block placing. */
	bool CanPushbackBlocks = true;
	/* Whether the player is allowed to see all entity name tags. */
	bool CanSeeAllNames = true;
	/* Whether the player is allowed to double jump. */
	bool CanDoubleJump = true;
	/* Maximum speed the entity can move at horizontally when CanSpeed is false. */
	Real32 MaxSpeedMultiplier = 1;

	/* Whether the player should slide after letting go of movement buttons in noclip.  */
	bool NoclipSlide = true;
	/* Whether the player has allowed the usage of fast double jumping abilities. */
	bool WOMStyleHacks = false;

	/* Whether the player currently has noclip on. */
	bool Noclip;
	/* Whether the player currently has fly mode active. */
	bool Flying;
	/* Whether the player is currently flying upwards. */
	bool FlyingUp;
	/* Whether the player is currently flying downwards. */
	bool FlyingDown;
	/* Whether the player is currently walking at base speed * speed multiplier. */
	bool Speeding;
	/* Whether the player is currently walking at base speed * 0.5 * speed multiplier. */
	bool HalfSpeeding;

	UInt8 HacksFlagsBuffer[String_BufferSize(128)];
	/* The actual hack flags usage string.*/
	String HacksFlags;
} HacksComponent;

bool HacksComponent_CanJumpHigher(HacksComponent* hacks) {
	return hacks->Enabled && hacks->CanAnyHacks && hacks->CanSpeed;
}

bool HacksComponent_Floating(HacksComponent* hacks) {
	return hacks->Noclip || hacks->Flying;
}

void ParseHorizontalSpeed(String joined) {
	int start = joined.IndexOf("horspeed=", StringComparison.OrdinalIgnoreCase);
	if (start < 0) return;
	start += 9;

	int end = joined.IndexOf(' ', start);
	if (end < 0) end = joined.Length;

	string num = joined.Substring(start, end - start);
	float value = 0;
	if (!Utils.TryParseDecimal(num, out value) || value <= 0) return;
	MaxSpeedMultiplier = value;
}

	void SetAllHacks(bool allowed) {
		CanAnyHacks = CanFly = CanNoclip = CanRespawn = CanSpeed =
			CanPushbackBlocks = CanUseThirdPersonCamera = allowed;
	}

static void ParseFlag(Action<bool> action, string joined, string flag) {
		if (joined.Contains("+" + flag)) {
			action(true);
		} else if (joined.Contains("-" + flag)) {
			action(false);
		}
	}

/* Sets the user type of this user. This is used to control permissions for grass,
bedrock, water and lava blocks on servers that don't support CPE block permissions. */
void SetUserType(HacksComponent* hacks, UInt8 value) {
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
	if (!CanFly || !Enabled) { Flying = false; FlyingDown = false; FlyingUp = false; }
	if (!CanNoclip || !Enabled) Noclip = false;
	if (!CanSpeed || !Enabled) { Speeding = false; HalfSpeeding = false; }
	CanDoubleJump = CanAnyHacks && Enabled && CanSpeed;
	CanSeeAllNames = CanAnyHacks && CanSeeAllNames;

	if (!CanUseThirdPersonCamera || !Enabled)
		game.CycleCamera();
}


	/* Updates ability to use hacks, and raises HackPermissionsChanged event. */
	/// <remarks> Parses hack flags specified in the motd and/or name of the server. </remarks>
	/// <remarks> Recognises +/-hax, +/-fly, +/-noclip, +/-speed, +/-respawn, +/-ophax, and horspeed=xyz </remarks>
	void UpdateHacksState() {
		SetAllHacks(true);
		if (HacksFlags == null) return;

		MaxSpeedMultiplier = 1;
		// By default (this is also the case with WoM), we can use hacks.
		if (HacksFlags.Contains("-hax")) SetAllHacks(false);

		ParseFlag(b = > CanFly = b, HacksFlags, "fly");
		ParseFlag(b = > CanNoclip = b, HacksFlags, "noclip");
		ParseFlag(b = > CanSpeed = b, HacksFlags, "speed");
		ParseFlag(b = > CanRespawn = b, HacksFlags, "respawn");

		if (UserType == 0x64)
			ParseFlag(b = > SetAllHacks(b), HacksFlags, "ophax");
		ParseHorizontalSpeed(HacksFlags);

		CheckHacksConsistency();
		game.Events.RaiseHackPermissionsChanged();
	}
}
}
