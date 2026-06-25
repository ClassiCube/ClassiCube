#include "../ExtMath.h"

// These instrinsics are able to use FSCA instruction
float Math_SinF(float x) { 
	return __builtin_sinf(x);
}

float Math_CosF(float x) { 
	return __builtin_cosf(x);
}
