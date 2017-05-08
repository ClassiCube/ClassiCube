#include "ExtMath.h"

Int32 Math_Floor(Real32 value) {
	Int32 valueI = (Int32)value;
	return value < valueI ? valueI - 1 : valueI;
}