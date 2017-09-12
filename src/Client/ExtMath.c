#include "ExtMath.h"

Int32 Math_Floor(Real32 value) {
	Int32 valueI = (Int32)value;
	return value < valueI ? valueI - 1 : valueI;
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