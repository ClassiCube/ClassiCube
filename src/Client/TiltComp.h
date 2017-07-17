#ifndef CS_TILTCOMP_H
#define CS_TILTCOMP_H
#include "Typedefs.h"
#include "Entity.h"
/* Entity component that performs tilt animation depending on movement speed and time.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Stores variables for performing tilt animation. */
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