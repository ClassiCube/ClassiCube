#ifndef CS_ANIMATEDCOMP_H
#define CS_ANIMATEDCOMP_H
#include "Typedefs.h"
#include "Vectors.h"
/* Entity component that performs model animation depending on movement speed and time.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Stores variables for performing model animation. */
typedef struct AnimatedComp_ {
	Real32 BobbingHor, BobbingVer, BobbingModel;
	Real32 WalkTime, Swing, BobStrength;
	Real32 WalkTimeO, WalkTimeN, SwingO, SwingN, BobStrengthO, BobStrengthN;

	Real32 LeftLegX, LeftLegZ, RightLegX, RightLegZ;
	Real32 LeftArmX, LeftArmZ, RightArmX, RightArmZ;
} AnimatedComp;

/* Initalises default values for an animated component. */
void AnimatedComp_Init(AnimatedComp* anim);

/* Calculates the next model animation state based on old and new position. */
void AnimatedComp_Update(AnimatedComp* anim, Vector3 oldPos, Vector3 newPos, Real64 delta, bool onGround);

/*  Calculates the interpolated state between the last and next model animation state. */
void AnimatedComp_GetCurrent(AnimatedComp* anim, Real32 t, bool calcHumanAnims);

void AnimatedComp_DoTilt(Real32* tilt, bool reduce);

static void AnimatedComp_CalcHumanAnim(AnimatedComp* anim, Real32 idleXRot, Real32 idleZRot);

static void AnimatedComp_PerpendicularAnim(AnimatedComp* anim, Real32 flapSpeed, Real32 idleXRot, Real32 idleZRot, bool left);
#endif