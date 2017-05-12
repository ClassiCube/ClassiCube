#include "ExtMath.h"

Int32 Math_Floor(Real32 value) {
	Int32 valueI = (Int32)value;
	return value < valueI ? valueI - 1 : valueI;
}

Int32 Math_CeilDiv(Int32 a, Int32 b) {
	return a / b + (a % b != 0 ? 1 : 0);
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