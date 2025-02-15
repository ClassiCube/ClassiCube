#include "ExtMath.h"
#include "Platform.h"
#include "Utils.h"

#define PI 3.141592653589793238462643383279502884197169399

static const cc_uint64 _DBL_NAN = 0x7FF8000000000000ULL;
#define DBL_NAN  *((double*)&_DBL_NAN)
static const cc_uint64 _POS_INF = 0x7FF0000000000000ULL;
#define POS_INF *((double*)&_POS_INF)


int Math_Floor(float value) {
	int valueI = (int)value;
	return valueI > value ? valueI - 1 : valueI;
}

int Math_Ceil(float value) {
	int valueI = (int)value;
	return valueI < value ? valueI + 1 : valueI;
}

int Math_ilog2(cc_uint32 value) {
	cc_uint32 r = 0;
	while (value >>= 1) r++;
	return r;
}

int Math_CeilDiv(int a, int b) {
	return a / b + (a % b != 0 ? 1 : 0);
}

int Math_Sign(float value) {
	if (value > 0.0f) return +1;
	if (value < 0.0f) return -1;
	return 0;
}

float Math_Lerp(float a, float b, float t) {
	return a + (b - a) * t;
}

float Math_ClampAngle(float degrees) {
	while (degrees >= 360.0f) degrees -= 360.0f;
	while (degrees < 0.0f)    degrees += 360.0f;
	return degrees;
}

float Math_LerpAngle(float leftAngle, float rightAngle, float t) {
	/* Need to potentially adjust a bit when interpolating some angles */
	/* Consider 350* --> 0*, we only want to interpolate across the 10* */
	/* But without adjusting for this case, we would interpolate back the whole 350* degrees */
	cc_bool invertLeft  = leftAngle  > 270.0f && rightAngle < 90.0f;
	cc_bool invertRight = rightAngle > 270.0f && leftAngle  < 90.0f;
	if (invertLeft)  leftAngle  = leftAngle  - 360.0f;
	if (invertRight) rightAngle = rightAngle - 360.0f;

	return Math_Lerp(leftAngle, rightAngle, t);
}

int Math_NextPowOf2(int value) {
	int next = 1;
	while (value > next) { next <<= 1; }
	return next;
}

cc_bool Math_IsPowOf2(int value) {
	return value != 0 && (value & (value - 1)) == 0;
}

float Math_Mod1(float x) { return x - (int)x; /* fmodf(x, 1); */ }


/*########################################################################################################################*
*-------------------------------------------------------Math intrinsics---------------------------------------------------*
*#########################################################################################################################*/
/* For abs(x) function */
#if defined CC_BUILD_AMIGA || defined CC_BUILD_SATURN
static int abs(int x) { return x < 0 ? -x : x; }
#else
#include <stdlib.h>
#endif

/* 32x/Saturn/GBA is missing these intrinsics */
#if defined CC_BUILD_32X || defined CC_BUILD_SATURN || defined CC_BUILD_GBA
#include "../third_party/fix16_sqrt.c"

float sqrtf(float x) {
	int32_t fp_x = (int32_t)(x * (1 << 16));
	fp_x = sqrt_fix16(fp_x);
	return (float)fp_x / (1 << 16);
}
#endif


#if defined CC_BUILD_PS1
	/* PS1 is missing these intrinsics */
	#include <psxgte.h>
	float Math_AbsF(float x)  { return __builtin_fabsf(x); }

	float Math_SqrtF(float x) { 
		int fp_x = (int)(x * (1 << 12));
		fp_x = SquareRoot12(fp_x);
		return (float)fp_x / (1 << 12);
	}
#elif defined __GNUC__
	/* Defined in .h using builtins */
#elif defined __TINYC__
	/* Older versions of TinyC don't support fabsf or sqrtf */
	/* Those can be used though if compiling with newer TinyC */
	/*  versions for a very small performance improvement */
	#include <math.h>

	float Math_AbsF(float x)  { return fabs(x); }
	float Math_SqrtF(float x) { return sqrt(x); }
#else
	#include <math.h>

	float Math_AbsF(float x)  { return fabsf(x); /* MSVC intrinsic */ }
	float Math_SqrtF(float x) { return sqrtf(x); /* MSVC intrinsic */ }
#endif

int Math_AbsI(int x) { return abs(x); /* MSVC intrinsic */ }


/*########################################################################################################################*
*--------------------------------------------------Random number generator------------------------------------------------*
*#########################################################################################################################*/
#define RND_VALUE (0x5DEECE66DULL)
#define RND_MASK ((1ULL << 48) - 1)

void Random_SeedFromCurrentTime(RNGState* rnd) {
	cc_uint64 now = Stopwatch_Measure();
	Random_Seed(rnd, (int)now);
}

void Random_Seed(RNGState* seed, int seedInit) {
	*seed = (seedInit ^ RND_VALUE) & RND_MASK;
}

int Random_Next(RNGState* seed, int n) {
	cc_int64 raw;
	int bits, val;

	if ((n & -n) == n) { /* i.e., n is a power of 2 */
		*seed = (*seed * RND_VALUE + 0xBLL) & RND_MASK;
		raw   = (cc_int64)(*seed >> (48 - 31));
		return (int)((n * raw) >> 31);
	}

	do {
		*seed = (*seed * RND_VALUE + 0xBLL) & RND_MASK;
		bits  = (int)(*seed >> (48 - 31));
		val   = bits % n;
	} while (bits - val + (n - 1) < 0);
	return val;
}

float Random_Float(RNGState* seed) {
	int raw;

	*seed = (*seed * RND_VALUE + 0xBLL) & RND_MASK;
	raw   = (int)(*seed >> (48 - 24));
	return raw / ((float)(1 << 24));
}


/*########################################################################################################################*
*--------------------------------------------------Trigonometric functions-----------------------------------------------*
*#########################################################################################################################*/
#if defined CC_BUILD_DREAMCAST
#include <math.h>

/* If don't have some code referencing libm, then gldc will fail to link with undefined reference to fabs */
/* TODO: Properly investigate this issue */
/* double make_dreamcast_build_compile(void) { fabs(4); } */

float Math_SinF(float x)   { return sinf(x); }
float Math_CosF(float x)   { return cosf(x); }
#elif defined CC_BUILD_NOFPU

// Source https://www.coranac.com/2009/07/sines
#define ISIN_QN	10
#define QA		12
#define ISIN_B	19900
#define	ISIN_C	3516

static CC_INLINE int isin_s4(int x) {
	int c, x2, y;

	c  = x << (30 - ISIN_QN);		// Semi-circle info into carry.
	x -= 1 << ISIN_QN;				// sine -> cosine calc

	x <<= (31 - ISIN_QN);			// Mask with PI
	x >>= (31 - ISIN_QN);			// Note: SIGNED shift! (to QN)
	x  *= x;
	x >>= (2 * ISIN_QN - 14);		// x=x^2 To Q14

	y = ISIN_B - (x * ISIN_C >> 14);// B - x^2*C
	y = (1 << QA) - (x * y >> 16);	// A - x^2*(B-x^2*C)

	return (c >= 0) ? y : (-y);
}

float Math_SinF(float angle) {
	int raw = (int)(angle * MATH_RAD2DEG * 4096 / 360);
	return isin_s4(raw) / 4096.0f;
}

float Math_CosF(float angle) {
	int raw = (int)(angle * MATH_RAD2DEG * 4096 / 360);
	raw += (1 << ISIN_QN); // add offset to calculate cos(x) instead of sin(x)
	return isin_s4(raw) / 4096.0f;
}

#else
/***** Caleb's Math functions *****/

/* This code implements the math functions sine, cosine, arctangent, the
 * exponential function, and the logarithmic function. The code uses techniques
 * exclusively described in the book "Computer Approximations" by John Fraser
 * Hart (1st Edition). Each function approximates their associated math function
 * the same way:
 *
 *   1. First, the function uses properties of the associated math function to
 *      reduce the input range to a small finite interval,
 *
 *   2. Second, the function calculates a polynomial, rational, or similar
 *      function that approximates the associated math function on that small
 *      finite interval to the desired accuracy. These polynomial, rational, or
 *      similar functions were calculated by the authors of "Computer
 *      Approximations" using the Remez algorithm and exist in the book's
 *      appendix.
 */

/* NOTE: NaN/Infinity checking was removed from Cos/Sin functions, */
/*  since ClassiCube does not care about the exact return value */
/*  from the mathematical functions anyways */

/* Global constants */
#define DIV_2_PI (1.0 / (2.0 * PI))

/* Calculates the floor of a double.
 */
static double Floord(double x) {
	if (x >= 0)
		return (double) ((int) x);
	return (double) (((int) x) - 1);
}

/************
 * Math_Sin *
 ************/

/* Calculates the 5th degree polynomial function SIN 2922 listed in the book's
 * appendix.
 *
 * Associated math function: sin(pi/6 * x)
 * Allowed input range: [0, 1]
 * Precision: 16.47
 */
static double SinStage1(double x) {
	const static double A[] = {
		.52359877559829885532,
		-.2392459620393377657e-1,
		.32795319441392666e-3,
		-.214071970654441e-5,
		.815113605169e-8,
		-.2020852964e-10,
	};

	double P = A[5];
	double x_2 = x * x;
	int i;

	for (i = 4; i >= 0; i--) {
		P *= x_2;
		P += A[i];
	}
	P *= x;
	return P;
}

/* Uses the property
 *   sin(x) = sin(x/3) * (3 - 4 * (sin(x/3))^2)
 * to reduce the input range of sin(x) to [0, pi/6].
 *
 * Associated math function: sin(2 * pi * x)
 * Allowed input range: [0, 0.25]
 */
static double SinStage2(double x) {
	double sin_6 = SinStage1(x * 4.0);
	return sin_6 * (3.0 - 4.0 * sin_6 * sin_6);
}

/* Uses the properties of sine to reduce the input range from [0, 2*pi] to [0,
 * pi/2].
 *
 * Associated math function: sin(2 * pi * x)
 * Allowed input range: [0, 1]
 */
static double SinStage3(double x) {
	if (x < 0.25)
		return SinStage2(x);
	if (x < 0.5)
		return SinStage2(0.5 - x);
	if (x < 0.75)
		return -SinStage2(x - 0.5);
	return -SinStage2(1.0 - x);
}

/* Since sine has a period of 2*pi, this function maps any real number to a
 * number from [0, 2*pi].
 *
 * Associated math function: sin(x)
 * Allowed input range: anything
 */
float Math_SinF(float x) {
	double x_div_pi;

	x_div_pi = x * DIV_2_PI;
	return (float)SinStage3(x_div_pi - Floord(x_div_pi));
}

/************
 * Math_Cos *
 ************/

/* This function works just like the above sine function, except it shifts the
 * input by pi/2, using the property cos(x) = sin(x + pi/2).
 *
 * Associated math function: cos(x)
 * Allowed input range: anything
 */
float Math_CosF(float x) {
	double x_div_pi_shifted;

	x_div_pi_shifted = x * DIV_2_PI + 0.25;
	return (float)SinStage3(x_div_pi_shifted - Floord(x_div_pi_shifted));
}
#endif


/*########################################################################################################################*
*--------------------------------------------------Transcendental functions-----------------------------------------------*
*#########################################################################################################################*/
#if defined CC_BUILD_DREAMCAST
#include <math.h>

double Math_Exp2(double x) { return exp2(x); }
double Math_Log2(double x) { return log2(x); }
#else
/***** Caleb's Math functions *****/

/* This code implements the math functions sine, cosine, arctangent, the
 * exponential function, and the logarithmic function. The code uses techniques
 * exclusively described in the book "Computer Approximations" by John Fraser
 * Hart (1st Edition). Each function approximates their associated math function
 * the same way:
 *
 *   1. First, the function uses properties of the associated math function to
 *      reduce the input range to a small finite interval,
 *
 *   2. Second, the function calculates a polynomial, rational, or similar
 *      function that approximates the associated math function on that small
 *      finite interval to the desired accuracy. These polynomial, rational, or
 *      similar functions were calculated by the authors of "Computer
 *      Approximations" using the Remez algorithm and exist in the book's
 *      appendix.
 */

/* Global constants */
static const double SQRT2 = 1.4142135623730950488016887242096980785696718753769;

/************
 * Math_Exp *
 ************/

/* Calculates the function EXPB 1067 listed in the book's appendix. It is of the
 * form
 *   (Q(x^2) + x*P(x^2)) / (Q(x^2) - x*P(x^2))
 *
 * Associated math function: 2^x
 * Allowed input range: [-1/2, 1/2]
 * Precision: 18.08
 */
static double Exp2Stage1(double x) {
	const double A_P[] = {
		.1513906799054338915894328e4,
		.20202065651286927227886e2,
		.23093347753750233624e-1,
	};

	const double A_Q[] = {
		.4368211662727558498496814e4,
		.233184211427481623790295e3,
		1.0,
	};

	double x_2 = x * x;
	double P, Q;
	int i;

	P = A_P[2];
	for (i = 1; i >= 0; i--) {
		P *= x_2;
		P += A_P[i];
	}
	P *= x;

	Q = A_Q[2];
	for (i = 1; i >= 0; i--) {
		Q *= x_2;
		Q += A_Q[i];
	}

	return (Q + P) / (Q - P);
}

/* Reduces the range of 2^x to [-1/2, 1/2] by using the property
 *   2^x = 2^(integer value) * 2^(fractional part).
 * 2^(integer value) can be calculated by directly manipulating the bits of the
 * double-precision floating point representation.
 *
 * Associated math function: 2^x
 * Allowed input range: anything
 */
double Math_Exp2(double x) {
	int x_int;
	union { double d; cc_uint64 i; } doi;

	if (x == POS_INF || x == DBL_NAN)
		return x;

	x_int = (int)x;

	if (x_int <= -1022)
		return 0.0;
	if (x_int > 1023)
		return POS_INF;

	if (x < 0)
		x_int--;

	doi.i = x_int + 1023;
	doi.i <<= 52;

	return doi.d * SQRT2 * Exp2Stage1(x - (double) x_int - 0.5);
}

/************
 * Math_Log *
 ************/

/* Calculates the 3rd/3rd degree rational function LOG2 2524 listed in the
 * book's appendix.
 *
 * Associated math function: log_2(x)
 * Allowed input range: [0.5, 1]
 * Precision: 8.32
 */
static double Log2Stage1(double x) {
	const double A_P[] = {
		-.205466671951e1,
		-.88626599391e1,
		.610585199015e1,
		.481147460989e1,
	};

	const double A_Q[] = {
		.353553425277,
		.454517087629e1,
		.642784209029e1,
		1.0,
	};

	double P, Q;
	int i;

	P = A_P[3];
	for (i = 2; i >= 0; i--) {
		P *= x;
		P += A_P[i];
	}

	Q = A_Q[3];
	for (i = 2; i >= 0; i--) {
		Q *= x;
		Q += A_Q[i];
	}

	return P / Q;
}

/* Reduces the range of log_2(x) by using the property that
 *   log_2(x) = (x's exponent part) + log_2(x's mantissa part)
 * So, by manipulating the bits of the double-precision floating point number
 * one can reduce the range of the logarithm function.
 *
 * Associated math function: log_2(x)
 * Allowed input range: anything
 */
double Math_Log2(double x) {
	union { double d; cc_uint64 i; } doi;
	int exponent;

	if (x == POS_INF)
		return POS_INF;

	if (x == DBL_NAN || x <= 0.0)
		return DBL_NAN;

	doi.d = x;
	exponent = (doi.i >> 52);
	exponent -= 1023;

	doi.i |= (((cc_uint64) 1023) << 52);
	doi.i &= ~(((cc_uint64) 1024) << 52);

	return exponent + Log2Stage1(doi.d);
}
#endif


// Approximation of atan2f using the Remez algorithm
//  https://math.stackexchange.com/a/1105038
float Math_Atan2f(float x, float y) {
	float ax, ay, a, s, r;

	if (x == 0) {
		if (y > 0) return  PI / 2.0f;
		if (y < 0) return -PI / 2.0f;
		return 0; /* Should probably be NaN */
	}
	
	ax = Math_AbsF(x);
	ay = Math_AbsF(y);

	a = (ax < ay) ? (ax / ay) : (ay / ax);
	s = a * a;
	r = ((-0.0464964749f * s + 0.15931422f) * s - 0.327622764f) * s * a + a;

	if (ay > ax) r = 1.57079637f - r;
	if (x < 0)   r = 3.14159274f - r;
	if (y < 0)   r = -r;
	return r;
}

double Math_Sin(double x) { return Math_SinF(x); }
double Math_Cos(double x) { return Math_CosF(x); }
