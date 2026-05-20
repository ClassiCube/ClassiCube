#include "../ExtMath.h"

// These are able to use fsca
float Math_SinF(float x) { 
	return __builtin_sinf(x);
}

float Math_CosF(float x) { 
	return __builtin_cosf(x);
}
