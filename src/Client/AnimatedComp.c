#include "AnimatedComp.h"
#include "ExtMath.h"
#include "GameProps.h"

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

#define armMax (60.0f * MATH_DEG2RAD)
#define legMax (80.0f * MATH_DEG2RAD)
#define idleMax (3.0f * MATH_DEG2RAD)
#define idleXPeriod (2.0f * MATH_PI / 5.0f)
#define idleZPeriod (2.0f * MATH_PI / 3.5f)

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

void AnimatedComp_DoTilt(Real32* tilt, bool reduce) {
	if (reduce) {
		(*tilt) *= 0.84f;
	} else {
		(*tilt) += 0.1f;
	}
	Math_Clamp(*tilt, 0.0f, 1.0f);
}


void AnimatedComp_CalcHumanAnim(AnimatedComp* anim, Real32 idleXRot, Real32 idleZRot) {
	AnimatedComp_PerpendicularAnim(anim, 0.23f, idleXRot, idleZRot, true);
	AnimatedComp_PerpendicularAnim(anim, 0.28f, idleXRot, idleZRot, false);
	anim->RightArmX = -anim->RightArmX; anim->RightArmZ = -anim->RightArmZ;
}

#define maxAngle (110 * MATH_DEG2RAD)
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