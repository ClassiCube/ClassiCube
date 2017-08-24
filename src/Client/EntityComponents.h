#ifndef CS_ENTITYCOMPS_H
#define CS_ENTITYCOMPS_H
#include "Typedefs.h"
#include "Vectors.h"
/* Contains various components for entities.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Entity component that performs model animation depending on movement speed and time. */
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


/* Entity component that performs tilt animation depending on movement speed and time. */
typedef struct TiltComp_ {
	Real32 TiltX, TiltY, VelTiltStrength;
	Real32 VelTiltStrengthO, VelTiltStrengthN;
} TiltComp;


/* Initalises default values for a tilt component. */
void TiltComp_Init(TiltComp* anim);
/* Calculates the next tilt animation state. */
void TiltComp_Update(TiltComp* anim, Real64 delta);
/*  Calculates the interpolated state between the last and next tilt animation state. */
void TiltComp_GetCurrent(TiltComp* anim, Real32 t);
#endif