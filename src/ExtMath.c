#include "ExtMath.h"
#include "Platform.h"
#include "Utils.h"
/* For abs(x) function */
#include <stdlib.h>

#ifndef __GNUC__
#include <math.h>
float Math_AbsF(float x)  { return fabsf(x); /* MSVC intrinsic */ }
float Math_SqrtF(float x) { return sqrtf(x); /* MSVC intrinsic */ }
#endif

float Math_Mod1(float x)  { return x - (int)x; /* fmodf(x, 1); */ }
int   Math_AbsI(int x)    { return abs(x); /* MSVC intrinsic */ }

float Math_SinF(float x) { return (float)Math_Sin(x); }
float Math_CosF(float x) { return (float)Math_Cos(x); }

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
	/* We have to cheat a bit for angles here */
	/* Consider 350* --> 0*, we only want to travel 10* */
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


/*########################################################################################################################*
*--------------------------------------------------Random number generator------------------------------------------------*
*#########################################################################################################################*/
#define RND_VALUE (0x5DEECE66DULL)
#define RND_MASK ((1ULL << 48) - 1)

void Random_SeedFromCurrentTime(RNGState* rnd) {
	TimeMS now = DateTime_CurrentUTC_MS();
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
*--------------------------------------------------Transcendental functions-----------------------------------------------*
*#########################################################################################################################*/
static const double SQRT2 = 1.4142135623730950488016887242096980785696718753769;
static const double LOGE2 = 0.6931471805599453094172321214581765680755001343602;

#ifdef CC_BUILD_DREAMCAST
#include <math.h>

/* If don't have some code referencing libm, then gldc will fail to link with undefined reference to fabs */
/* TODO: Properly investigate this issue */
/* double make_dreamcast_build_compile(void) { fabs(4); } */

double Math_Sin(double x)  { return sin(x); }
double Math_Cos(double x)  { return cos(x); }
double Math_Exp2(double x) { return exp2(x); }
double Math_Log2(double x) { return log2(x); }

double Math_Atan2(double x, double y) { return atan2(y, x); }
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
#define PI 3.141592653589793238462643383279502884197169399
#define DIV_2_PI (1.0 / (2.0 * PI))

static const cc_uint64 _DBL_NAN = 0x7FF8000000000000ULL;
#define DBL_NAN  *((double*)&_DBL_NAN)
static const cc_uint64 _POS_INF = 0x7FF0000000000000ULL;
#define POS_INF *((double*)&_POS_INF)
static const cc_uint64 _NEG_INF = 0xFFF0000000000000ULL;
#define NEG_INF *((double*)&_NEG_INF)

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
	const double A[] = {
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
double Math_Sin(double x) {
	double x_div_pi;

	x_div_pi = x * DIV_2_PI;
	return SinStage3(x_div_pi - Floord(x_div_pi));
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
double Math_Cos(double x) {
	double x_div_pi_shifted;

	x_div_pi_shifted = x * DIV_2_PI + 0.25;
	return SinStage3(x_div_pi_shifted - Floord(x_div_pi_shifted));
}

/**************
 * Math_Atan2 *
 **************/

/* Calculates the 5th degree polynomial ARCTN 4903 listed in the book's
 * appendix.
 *
 * Associated math function: arctan(x)
 * Allowed input range: [0, tan(pi/32)]
 * Precision: 16.52
 */
static double AtanStage1(double x) {
	const double A[] = {
		.99999999999969557,
		-.3333333333318,
		.1999999997276,
		-.14285702288,
		.11108719478,
		-.8870580341e-1,
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

/* This function finds out in which partition the non-negative real number x
 * resides out of 8 partitions, which are precomputed. It then uses the
 * following law:
 *
 *   t = x_i^{-1} - (x_i^{-2} + 1)/(x_i^{-1} + x)
 *   arctan(x) = arctan(x_i) + arctan(t)
 *
 * where x_i = tan((2i - 2)*pi/32) and i is the partition number. The value of t
 * is guaranteed to be between [-tan(pi/32), tan(pi/32)].
 *
 * Associated math function: arctan(x)
 * Allowed input range: [0, infinity]
 */
static double AtanStage2(double x) {
	const double X_i[] = {
		0.0,
		0.0984914033571642477671304050090839155018329620361328125,
		0.3033466836073424044428747947677038609981536865234375,
		0.53451113595079158269385288804187439382076263427734375,
		0.82067879082866024287312711749109439551830291748046875,
		1.218503525587976366040265929768793284893035888671875,
		1.8708684117893887854933154812897555530071258544921875,
		3.29655820893832096629694206058047711849212646484375,
		(float)(1e+300 * 1e+300) /* Infinity */
	};

	const double div_x_i[] = {
		0,
		0,
		5.02733949212584807497705696732737123966217041015625,
		2.41421356237309492343001693370752036571502685546875,
		1.496605762665489169904731170390732586383819580078125,
		1.0000000000000002220446049250313080847263336181640625,
		0.66817863791929898997778991542872972786426544189453125,
		0.414213562373095089963470627481001429259777069091796875,
		0.1989123673796580893391450217677629552781581878662109375,
	};

	const double div_x_i_2_plus_1[] = {
		0,
		0,
		26.2741423690881816810360760428011417388916015625,
		6.8284271247461898468600338674150407314300537109375,
		3.23982880884355051165357508580200374126434326171875,
		2.000000000000000444089209850062616169452667236328125,
		1.446462692171689656817079594475217163562774658203125,
		1.1715728752538099310953612075536511838436126708984375,
		1.0395661298965801488947136022034101188182830810546875,
	};

	int L = 0;
	int R = 8;
	double t;

	while (R - L > 1) {
		int m = (L + R) / 2;
		if (X_i[m] <= x)
			L = m;
		else if (X_i[m] > x)
			R = m;
	}

	if (R <= 1)
		return AtanStage1(x);

	t = div_x_i[R] - div_x_i_2_plus_1[R] / (div_x_i[R] + x);
	if (t >= 0)
		return (2 * R - 2) * PI / 32.0 + AtanStage1(t);

	return (2 * R - 2) * PI / 32.0 - AtanStage1(-t);
}

/* Uses the property arctan(x) = -arctan(-x).
 *
 * Associated math function: arctan(x)
 * Allowed input range: anything
 */
static double Atan(double x) {
	if (x == DBL_NAN)
		return DBL_NAN;
	if (x == NEG_INF)
		return -PI / 2.0;
	if (x == POS_INF)
		return PI / 2.0;
	if (x >= 0)
		return AtanStage2(x);
	return -AtanStage2(-x);
}

/* Implements the function atan2 using Atan.
 *
 * Associated math function: atan2(y, x)
 * Allowed input range: anything
 */
double Math_Atan2(double x, double y) {
	if (x > 0)
		return Atan(y / x);
	if (x < 0) {
		if (y >= 0)
			return Atan(y / x) + PI;
		return Atan(y / x) - PI;
	}
	if (y > 0)
		return PI / 2.0;
	if (y < 0)
		return -PI / 2.0;
	return DBL_NAN;
}

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
	if (x == NEG_INF)
		return 0.0;

	x_int = (int) x;

	if (x < 0)
		x_int--;

	if (x_int < -1022)
		return 0.0;
	if (x_int > 1023)
		return POS_INF;

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

	if (x == NEG_INF || x == DBL_NAN || x <= 0.0)
		return DBL_NAN;

	doi.d = x;
	exponent = (doi.i >> 52);
	exponent -= 1023;

	doi.i |= (((cc_uint64) 1023) << 52);
	doi.i &= ~(((cc_uint64) 1024) << 52);

	return exponent + Log2Stage1(doi.d);
}

#endif

/* Uses the property that
 *   log_e(x) = log_2(x) * log_e(2).
 *
 * Associated math function: log_e(x)
 * Allowed input range: anything
 */
double Math_Log(double x) {
	return Math_Log2(x) * LOGE2;
}
