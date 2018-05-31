#include "ExtMath.h"
#include <math.h>

/* TODO: Replace with own functions that don't rely on stdlib */

Real32 Math_AbsF(Real32 x)  { return fabsf(x); /* MSVC intrinsic */ }
Real32 Math_SqrtF(Real32 x) { return sqrtf(x); /* MSVC intrinsic */ }
Real32 Math_Mod1(Real32 x)  { return x - (Int32)x; /* fmodf(x, 1); */ }
Int32 Math_AbsI(Int32 x)    { return abs(x); /* MSVC intrinsic */ }

Real64 Math_Sin(Real64 x) { return sin(x); }
Real64 Math_Cos(Real64 x) { return cos(x); }
Real64 Math_Log(Real64 x) { return log(x); }
Real64 Math_Exp(Real64 x) { return exp(x); }

Real32 Math_SinF(Real32 x) { return (Real32)Math_Sin(x); }
Real32 Math_CosF(Real32 x) { return (Real32)Math_Cos(x); }

Int32 Math_Floor(Real32 value) {
	Int32 valueI = (Int32)value;
	return valueI > value ? valueI - 1 : valueI;
}

Int32 Math_Ceil(Real32 value) {
	Int32 valueI = (Int32)value;
	return valueI < value ? valueI + 1 : valueI;
}

Int32 Math_Log2(Int32 value) {
	Int32 shift = 0;
	while (value > 1) { shift++; value >>= 1; }
	return shift;
}

Int32 Math_CeilDiv(Int32 a, Int32 b) {
	return a / b + (a % b != 0 ? 1 : 0);
}

Int32 Math_Sign(Real32 value) {
	if (value > 0.0f) return +1;
	if (value < 0.0f) return -1;
	return 0;
}

Real32 Math_Lerp(Real32 a, Real32 b, Real32 t) {
	return a + (b - a) * t;
}

Real32 Math_LerpAngle(Real32 leftAngle, Real32 rightAngle, Real32 t) {
	/* we have to cheat a bit for angles here. */
	/* Consider 350* --> 0*, we only want to travel 10*, */
	/* but without adjusting for this case, we would interpolate back the whole 350* degrees.*/
	bool invertLeft = leftAngle > 270.0f && rightAngle < 90.0f;
	bool invertRight = rightAngle > 270.0f && leftAngle < 90.0f;
	if (invertLeft) leftAngle = leftAngle - 360.0f;
	if (invertRight) rightAngle = rightAngle - 360.0f;

	return Math_Lerp(leftAngle, rightAngle, t);
}

Int32 Math_NextPowOf2(Int32 value) {
	Int32 next = 1;
	while (value > next)
		next <<= 1;
	return next;
}

bool Math_IsPowOf2(Int32 value) {
	return value != 0 && (value & (value - 1)) == 0;
}

Int32 Math_AccumulateWheelDelta(Real32* accmulator, Real32 delta) {
	/* Some mice may use deltas of say (0.2, 0.2, 0.2, 0.2, 0.2) */
	/* We must use rounding at final step, not at every intermediate step. */
	*accmulator += delta;
	Int32 steps = (Int32)*accmulator;
	*accmulator -= steps;
	return steps;
}

/* Not the most precise Tan(x), but within 10^-15 of answer, so good enough for here */
Real64 Math_FastTan(Real64 angle) {
	Real64 cosA = Math_Cos(angle), sinA = Math_Sin(angle);
	if (cosA < -0.00000001 || cosA > 0.00000001) return sinA / cosA;

	/* tan line is parallel to y axis, infinite gradient */
	Int32 sign = Math_Sign(sinA);
	if (cosA != 0.0) sign *= Math_Sign(cosA);
	return sign * MATH_POS_INF;
}

Real64 Math_FastLog(Real64 x) {
	/* x = 2^exp * mantissa */
	/* so log(x) = log(2^exp) + log(mantissa) */
	/* so log(x) = exp*log(2) + log(mantissa) */

	/* now need to work out log(mantissa) */
}

Real64 Math_FastExp(Real64 x) {
	/* let x = k*log(2) + f, where k is integer */
	/* so exp(x) = exp(k*log(2)) * exp(f) */
	/* so exp(x) = exp(log(2^k)) * exp(f) */
	/* so exp(x) = 2^k           * exp(f) */

	/* now need to work out exp(f) */
}