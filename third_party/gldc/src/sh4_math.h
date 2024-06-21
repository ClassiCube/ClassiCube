// ---- sh4_math.h - SH7091 Math Module ----
//
// Version 1.1.3
//
// This file is part of the DreamHAL project, a hardware abstraction library
// primarily intended for use on the SH7091 found in hardware such as the SEGA
// Dreamcast game console.
//
// This math module is hereby released into the public domain in the hope that it
// may prove useful. Now go hit 60 fps! :)
//
// --Moopthehedgehog
//

// Notes:
// - GCC 4 users have a different return type for the fsca functions due to an
//  internal compiler error regarding complex numbers; no issue under GCC 9.2.0
// - Using -m4 instead of -m4-single-only completely breaks the matrix and
//  vector operations
// - Function inlining must be enabled and not blocked by compiler options such
//  as -ffunction-sections, as blocking inlining will result in significant
//  performance degradation for the vector and matrix functions employing a
//  RETURN_VECTOR_STRUCT return type. I have added compiler hints and attributes
//  "static inline __attribute__((always_inline))" to mitigate this, so in most
//  cases the functions should be inlined regardless. If in doubt, check the
//  compiler asm output!
//

#ifndef __SH4_MATH_H_
#define __SH4_MATH_H_

#define GNUC_FSCA_ERROR_VERSION 4

//
// Fast SH4 hardware math functions
//
//
// High-accuracy users beware, the fsrra functions have an error of +/- 2^-21
// per http://www.shared-ptr.com/sh_insns.html
//

//==============================================================================
// Definitions
//==============================================================================
//
// Structures, useful definitions, and reference comments
//

// Front matrix format:
//
//    FV0 FV4 FV8  FV12
//    --- --- ---  ----
//  [ fr0 fr4 fr8  fr12 ]
//  [ fr1 fr5 fr9  fr13 ]
//  [ fr2 fr6 fr10 fr14 ]
//  [ fr3 fr7 fr11 fr15 ]
//
// Back matrix, XMTRX, is similar, although it has no FVn vector groups:
//
//  [ xf0 xf4 xf8  xf12 ]
//  [ xf1 xf5 xf9  xf13 ]
//  [ xf2 xf6 xf10 xf14 ]
//  [ xf3 xf7 xf11 xf15 ]
//

typedef struct __attribute__((aligned(32))) {
  float fr0;
  float fr1;
  float fr2;
  float fr3;
  float fr4;
  float fr5;
  float fr6;
  float fr7;
  float fr8;
  float fr9;
  float fr10;
  float fr11;
  float fr12;
  float fr13;
  float fr14;
  float fr15;
} ALL_FLOATS_STRUCT;

// Return structs should be defined locally so that GCC optimizes them into
// register usage instead of memory accesses.
typedef struct {
  float z1;
  float z2;
  float z3;
  float z4;
} RETURN_VECTOR_STRUCT;

#if __GNUC__ <= GNUC_FSCA_ERROR_VERSION
typedef struct {
  float sine;
  float cosine;
} RETURN_FSCA_STRUCT;
#endif

// Identity Matrix
//
//    FV0 FV4 FV8 FV12
//    --- --- --- ----
//  [  1   0   0   0  ]
//  [  0   1   0   0  ]
//  [  0   0   1   0  ]
//  [  0   0   0   1  ]
//

static const ALL_FLOATS_STRUCT MATH_identity_matrix = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};

// Constants
#define MATH_pi 3.14159265358979323846264338327950288419716939937510f
#define MATH_e 2.71828182845904523536028747135266249775724709369995f
#define MATH_phi 1.61803398874989484820458683436563811772030917980576f

//==============================================================================
// Basic math functions
//==============================================================================
//
// The following functions are available.
// Please see their definitions for other usage info, otherwise they may not
// work for you.
//
/*
  // |x|
  float MATH_fabs(float x)

  // sqrt(x)
  float MATH_fsqrt(float x)

  // a*b+c
  float MATH_fmac(float a, float b, float c)

  // fminf() - return the min of two floats
  // This doesn't check for NaN
  float MATH_Fast_Fminf(float a, float b)

  // fmaxf() - return the max of two floats
  // This doesn't check for NaN
  float MATH_Fast_Fmaxf(float a, float b)

  // Fast floorf() - return the nearest integer <= x as a float
  // This doesn't check for NaN
  float MATH_Fast_Floorf(float x)

  // Fast ceilf() - return the nearest integer >= x as a float
  // This doesn't check for NaN
  float MATH_Fast_Ceilf(float x)

  // Very fast floorf() - return the nearest integer <= x as a float
  // Inspired by a cool trick I came across here:
  // https://www.codeproject.com/Tips/700780/Fast-floor-ceiling-functions
  // This doesn't check for NaN
  float MATH_Very_Fast_Floorf(float x)

  // Very fast ceilf() - return the nearest integer >= x as a float
  // Inspired by a cool trick I came across here:
  // https://www.codeproject.com/Tips/700780/Fast-floor-ceiling-functions
  // This doesn't check for NaN
  float MATH_Very_Fast_Ceilf(float x)
*/

// |x|
// This one works on ARM and x86, too!
static inline __attribute__((always_inline)) float MATH_fabs(float x)
{
  asm volatile ("fabs %[floatx]\n"
    : [floatx] "+f" (x) // outputs, "+" means r/w
    : // no inputs
    : // no clobbers
  );

  return x;
}

// sqrt(x)
// This one works on ARM and x86, too!
// NOTE: There is a much faster version (MATH_Fast_Sqrt()) in the fsrra section of
// this file. Chances are you probably want that one.
static inline __attribute__((always_inline)) float MATH_fsqrt(float x)
{
  asm volatile ("fsqrt %[floatx]\n"
    : [floatx] "+f" (x) // outputs, "+" means r/w
    : // no inputs
    : // no clobbers
  );

  return x;
}

// a*b+c
static inline __attribute__((always_inline)) float MATH_fmac(float a, float b, float c)
{
  asm volatile ("fmac fr0, %[floatb], %[floatc]\n"
    : [floatc] "+f" (c) // outputs, "+" means r/w
    : "w" (a), [floatb] "f" (b) // inputs
    : // no clobbers
  );

  return c;
}

// Fast fminf() - return the min of two floats
// This doesn't check for NaN
static inline __attribute__((always_inline)) float MATH_Fast_Fminf(float a, float b)
{
  float output_float;

  asm volatile (
    "fcmp/gt %[floata], %[floatb]\n\t" // b > a (NaN evaluates to !GT; 0 -> T)
    "bt.s 1f\n\t" // yes, a is smaller
    " fmov %[floata], %[float_out]\n\t" // so return a
    "fmov %[floatb], %[float_out]\n" // no, either b is smaller or they're equal and it doesn't matter
  "1:\n"
    : [float_out] "=&f" (output_float) // outputs
    : [floata] "f" (a), [floatb] "f" (b) // inputs
    : "t" // clobbers
  );

  return output_float;
}

// Fast fmaxf() - return the max of two floats
// This doesn't check for NaN
static inline __attribute__((always_inline)) float MATH_Fast_Fmaxf(float a, float b)
{
  float output_float;

  asm volatile (
    "fcmp/gt %[floata], %[floatb]\n\t" // b > a (NaN evaluates to !GT; 0 -> T)
    "bt.s 1f\n\t" // yes, a is smaller
    " fmov %[floatb], %[float_out]\n\t" // so return b
    "fmov %[floata], %[float_out]\n" // no, either a is bigger or they're equal and it doesn't matter
  "1:\n"
    : [float_out] "=&f" (output_float) // outputs
    : [floata] "f" (a), [floatb] "f" (b) // inputs
    : "t" // clobbers
  );

  return output_float;
}

// Fast floorf() - return the nearest integer <= x as a float
// This doesn't check for NaN
static inline __attribute__((always_inline)) float MATH_Fast_Floorf(float x)
{
  float output_float;

  // To hold -1.0f
  float minus_one;

  asm volatile (
    "fldi1 %[minus_1]\n\t"
    "fneg %[minus_1]\n\t"
    "fcmp/gt %[minus_1], %[floatx]\n\t" // x >= 0
    "ftrc %[floatx], fpul\n\t" // convert float to int
    "bt.s 1f\n\t"
    " float fpul, %[float_out]\n\t" // convert int to float
    "fadd %[minus_1], %[float_out]\n" // if input x < 0, subtract 1.0
  "1:\n"
    : [minus_1] "=&f" (minus_one), [float_out] "=f" (output_float)
    : [floatx] "f" (x)
    : "fpul", "t"
  );

  return output_float;
}

// Fast ceilf() - return the nearest integer >= x as a float
// This doesn't check for NaN
static inline __attribute__((always_inline)) float MATH_Fast_Ceilf(float x)
{
  float output_float;

  // To hold 0.0f and 1.0f
  float zero_one;

  asm volatile (
    "fldi0 %[zero_1]\n\t"
    "fcmp/gt %[zero_1], %[floatx]\n\t" // x > 0
    "ftrc %[floatx], fpul\n\t" // convert float to int
    "bf.s 1f\n\t"
    " float fpul, %[float_out]\n\t" // convert int to float
    "fldi1 %[zero_1]\n\t"
    "fadd %[zero_1], %[float_out]\n" // if input x > 0, add 1.0
  "1:\n"
    : [zero_1] "=&f" (zero_one), [float_out] "=f" (output_float)
    : [floatx] "f" (x)
    : "fpul", "t"
  );

  return output_float;
}

// Very fast floorf() - return the nearest integer <= x as a float
// Inspired by a cool trick I came across here:
// https://www.codeproject.com/Tips/700780/Fast-floor-ceiling-functions
// This doesn't check for NaN
static inline __attribute__((always_inline)) float MATH_Very_Fast_Floorf(float x)
{
  float output_float;
  unsigned int scratch_reg;
  unsigned int scratch_reg2;

  // 0x4f000000 == 2^31 in float -- 0x4f << 24 is INT_MAX + 1.0f
  // 0x80000000 == -2^31 == INT_MIN == -(INT_MAX + 1.0f)

  // floor = (float)( (int)(x + (float)2^31) - 2^31)

  asm volatile (
    "mov #0x4f, %[scratch]\n\t" // Build float INT_MAX + 1 as a float using only regs (EX)
    "shll16 %[scratch]\n\t" // (EX)
    "shll8 %[scratch]\n\t" // (EX)
    "lds %[scratch], fpul\n\t" // move float INT_MAX + 1 to float regs (LS)
    "mov #1, %[scratch2]\n\t" // Build INT_MIN from scratch in parallel (EX)
    "fsts fpul, %[float_out]\n\t" // (LS)
    "fadd %[floatx], %[float_out]\n\t" // float-add float INT_MAX + 1 to x (FE)
    "rotr %[scratch2]\n\t" // rotate the 1 in bit 0 from LSB to MSB for INT_MIN, clobber T (EX)
    "ftrc %[float_out], fpul\n\t" // convert float to int (FE) -- ftrc -> sts is special combo
    "sts fpul, %[scratch]\n\t" // move back to int regs (LS)
    "add %[scratch2], %[scratch]\n\t" // Add INT_MIN to int (EX)
    "lds %[scratch], fpul\n\t" // (LS) -- lds -> float is a special combo
    "float fpul, %[float_out]\n" // convert back to float (FE)
    : [scratch] "=&r" (scratch_reg), [scratch2] "=&r" (scratch_reg2), [float_out] "=&f" (output_float)
    : [floatx] "f" (x)
    : "fpul", "t"
  );

  return output_float;
}

// Very fast ceilf() - return the nearest integer >= x as a float
// Inspired by a cool trick I came across here:
// https://www.codeproject.com/Tips/700780/Fast-floor-ceiling-functions
// This doesn't check for NaN
static inline __attribute__((always_inline)) float MATH_Very_Fast_Ceilf(float x)
{
  float output_float;
  unsigned int scratch_reg;
  unsigned int scratch_reg2;

  // 0x4f000000 == 2^31 in float -- 0x4f << 24 is INT_MAX + 1.0f
  // 0x80000000 == -2^31 == INT_MIN == -(INT_MAX + 1.0f)

  // Ceiling is the inverse of floor such that f^-1(x) = -f(-x)
  // To make very fast ceiling have as wide a range as very fast floor,
  // use this property to subtract x from INT_MAX + 1 and get the negative of the
  // ceiling, and then negate the final output. This allows ceiling to use
  // -2^31 and have the same range as very fast floor.

  // Given:
  // floor = (float)( (int)(x + (float)2^31) - 2^31 )
  // We can do:
  // ceiling = -( (float)( (int)((float)2^31 - x) - 2^31 ) )
  // or (slower on SH4 since 'fneg' is faster than 'neg'):
  // ceiling = (float) -( (int)((float)2^31 - x) - 2^31 )
  // Since mathematically these functions are related by f^-1(x) = -f(-x).

  asm volatile (
    "mov #0x4f, %[scratch]\n\t" // Build float INT_MAX + 1 as a float using only regs (EX)
    "shll16 %[scratch]\n\t" // (EX)
    "shll8 %[scratch]\n\t" // (EX)
    "lds %[scratch], fpul\n\t" // move float INT_MAX + 1 to float regs (LS)
    "mov #1, %[scratch2]\n\t" // Build INT_MIN from scratch in parallel (EX)
    "fsts fpul, %[float_out]\n\t" // (LS)
    "fsub %[floatx], %[float_out]\n\t" // float-sub x from float INT_MAX + 1 (FE)
    "rotr %[scratch2]\n\t" // rotate the 1 in bit 0 from LSB to MSB for INT_MIN, clobber T (EX)
    "ftrc %[float_out], fpul\n\t" // convert float to int (FE) -- ftrc -> sts is special combo
    "sts fpul, %[scratch]\n\t" // move back to int regs (LS)
    "add %[scratch2], %[scratch]\n\t" // Add INT_MIN to int (EX)
    "lds %[scratch], fpul\n\t" // (LS) -- lds -> float is a special combo
    "float fpul, %[float_out]\n\t" // convert back to float (FE)
    "fneg %[float_out]\n"
    : [scratch] "=&r" (scratch_reg), [scratch2] "=&r" (scratch_reg2), [float_out] "=&f" (output_float)
    : [floatx] "f" (x)
    : "fpul", "t"
  );

  return output_float;
}

//==============================================================================
// Fun with fsrra, which does 1/sqrt(x) in one cycle
//==============================================================================
//
// Error of 'fsrra' is +/- 2^-21 per http://www.shared-ptr.com/sh_insns.html
//
// The following functions are available.
// Please see their definitions for other usage info, otherwise they may not
// work for you.
//
/*
  // 1/sqrt(x)
  float MATH_fsrra(float x)

  // 1/x
  float MATH_Fast_Invert(float x)

  // A faster divide than the 'fdiv' instruction
  float MATH_Fast_Divide(float numerator, float denominator)

  // A faster square root then the 'fsqrt' instruction
  float MATH_Fast_Sqrt(float x)

  // Standard, accurate, and slow float divide. Use this if MATH_Fast_Divide() gives you issues.
  float MATH_Slow_Divide(float numerator, float denominator)
*/

// 1/sqrt(x)
static inline __attribute__((always_inline)) float MATH_fsrra(float x)
{
  asm volatile ("fsrra %[one_div_sqrt]\n"
  : [one_div_sqrt] "+f" (x) // outputs, "+" means r/w
  : // no inputs
  : // no clobbers
  );

  return x;
}

// 1/x
// 1.0f / sqrt(x^2)
static inline __attribute__((always_inline)) float MATH_Fast_Invert(float x)
{
  int neg = 0;

  if(x < 0.0f)
  {
    neg = 1;
  }

  x = MATH_fsrra(x*x); // 1.0f / sqrt(x^2)

  if(neg)
  {
    return -x;
  }
  else
  {
    return x;
  }
}

// It's faster to do this than to use 'fdiv'.
// Only fdiv can do doubles, however.
// Of course, not having to divide at all is generally the best way to go. :P
static inline __attribute__((always_inline)) float MATH_Fast_Divide(float numerator, float denominator)
{
  denominator = MATH_Fast_Invert(denominator);
  return numerator * denominator;
}

// fast sqrt(x)
// Crazy thing: invert(fsrra(x)) is actually about 3x faster than fsqrt.
static inline __attribute__((always_inline)) float MATH_Fast_Sqrt(float x)
{
  return MATH_Fast_Invert(MATH_fsrra(x));
}

// Standard, accurate, and slow float divide. Use this if MATH_Fast_Divide() gives you issues.
// This DOES work on negatives.
static inline __attribute__((always_inline)) float MATH_Slow_Divide(float numerator, float denominator)
{
  asm volatile ("fdiv %[div_denom], %[div_numer]\n"
  : [div_numer] "+f" (numerator) // outputs, "+" means r/w
  : [div_denom] "f" (denominator) // inputs
  : // clobbers
  );

  return numerator;
}

//==============================================================================
// Fun with fsca, which does simultaneous sine and cosine in 3 cycles
//==============================================================================
//
// NOTE: GCC 4.7 has a bug that prevents it from working with fsca and complex
// numbers in m4-single-only mode, so GCC 4 users will get a RETURN_FSCA_STRUCT
// instead of a _Complex float. This may be much slower in some instances.
//
// VERY IMPORTANT USAGE INFORMATION (sine and cosine functions):
//
// Due to the nature in which the fsca instruction behaves, you MUST do the
// following in your code to get sine and cosine from these functions:
//
//  _Complex float sine_cosine = [Call the fsca function here]
//  float sine_value = __real__ sine_cosine;
//  float cosine_value = __imag__ sine_cosine;
//  Your output is now in sine_value and cosine_value.
//
// This is necessary because fsca outputs both sine and cosine simultaneously
// and uses a double register to do so. The fsca functions do not actually
// return a double--they return two floats--and using a complex float here is
// just a bit of hacking the C language to make GCC do something that's legal in
// assembly according to the SH4 calling convention (i.e. multiple return values
// stored in floating point registers FR0-FR3). This is better than using a
// struct of floats for optimization purposes--this will operate at peak
// performance even at -O0, whereas a struct will not be fast at low
// optimization levels due to memory accesses.
//
// Technically you may be able to use the complex return values as a complex
// number if you wanted to, but that's probably not what you're after and they'd
// be flipped anyways (in mathematical convention, sine is the imaginary part).
//

// Notes:
// - From http://www.shared-ptr.com/sh_insns.html:
//      The input angle is specified as a signed fraction in twos complement.
//      The result of sin and cos is a single-precision floating-point number.
//      0x7FFFFFFF to 0x00000001: 360×2^15−360/2^16 to 360/2^16 degrees
//      0x00000000: 0 degree
//      0xFFFFFFFF to 0x80000000: −360/2^16 to −360×2^15 degrees
// - fsca format is 2^16 is 360 degrees, so a value of 1 is actually
//  1/182.044444444 of a degree or 1/10430.3783505 of a radian
// - fsca does a %360 automatically for values over 360 degrees
//
// Also:
// In order to make the best use of fsca units, a program must expect them from
// the outset and not "make them" by dividing radians or degrees to get them,
// otherwise it's just giving the 'fsca' instruction radians or degrees!
//

// The following functions are available.
// Please see their definitions for other usage info, otherwise they may not
// work for you.
//
/*
  // For integer input in native fsca units (fastest)
  _Complex float MATH_fsca_Int(unsigned int input_int)

  // For integer input in degrees
  _Complex float MATH_fsca_Int_Deg(unsigned int input_int)

  // For integer input in radians
  _Complex float MATH_fsca_Int_Rad(unsigned int input_int)

  // For float input in native fsca units
  _Complex float MATH_fsca_Float(float input_float)

  // For float input in degrees
  _Complex float MATH_fsca_Float_Deg(float input_float)

  // For float input in radians
  _Complex float MATH_fsca_Float_Rad(float input_float)
*/

//------------------------------------------------------------------------------
#if __GNUC__ <= GNUC_FSCA_ERROR_VERSION
//------------------------------------------------------------------------------
//
// This set of fsca functions is specifically for old versions of GCC.
// See later for functions for newer versions of GCC.
//

//
// Integer input (faster)
//

// For int input, input_int is in native fsca units (fastest)
static inline __attribute__((always_inline)) RETURN_FSCA_STRUCT MATH_fsca_Int(unsigned int input_int)
{
  register float __sine __asm__("fr0");
  register float __cosine __asm__("fr1");

  asm volatile ("lds %[input_number], FPUL\n\t" // load int from register (1 cycle)
    "fsca FPUL, DR0\n" // 3 cycle simultaneous sine/cosine
    : "=w" (__sine), "=f" (__cosine) // outputs
    : [input_number] "r" (input_int)  // inputs
    : "fpul" // clobbers
  );

  RETURN_FSCA_STRUCT output = {__sine, __cosine};
  return output;
}

// For int input, input_int is in degrees
static inline __attribute__((always_inline)) RETURN_FSCA_STRUCT MATH_fsca_Int_Deg(unsigned int input_int)
{
  // normalize whole number input degrees to fsca format
  input_int = ((1527099483ULL * input_int) >> 23);

  register float __sine __asm__("fr0");
  register float __cosine __asm__("fr1");

  asm volatile ("lds %[input_number], FPUL\n\t" // load int from register (1 cycle)
    "fsca FPUL, DR0\n" // 3 cycle simultaneous sine/cosine
    : "=w" (__sine), "=f" (__cosine) // outputs
    : [input_number] "r" (input_int)  // inputs
    : "fpul" // clobbers
  );

  RETURN_FSCA_STRUCT output = {__sine, __cosine};
  return output;
}

// For int input, input_int is in radians
static inline __attribute__((always_inline)) RETURN_FSCA_STRUCT MATH_fsca_Int_Rad(unsigned int input_int)
{
  // normalize whole number input rads to fsca format
  input_int = ((2734261102ULL * input_int) >> 18);

  register float __sine __asm__("fr0");
  register float __cosine __asm__("fr1");

  asm volatile ("lds %[input_number], FPUL\n\t" // load int from register (1 cycle)
    "fsca FPUL, DR0\n" // 3 cycle simultaneous sine/cosine
    : "=w" (__sine), "=f" (__cosine) // outputs
    : [input_number] "r" (input_int)  // inputs
    : "fpul" // clobbers
  );

  RETURN_FSCA_STRUCT output = {__sine, __cosine};
  return output;
}

//
// Float input (slower)
//

// For float input, input_float is in native fsca units
static inline __attribute__((always_inline)) RETURN_FSCA_STRUCT MATH_fsca_Float(float input_float)
{
  register float __sine __asm__("fr0");
  register float __cosine __asm__("fr1");

  asm volatile ("ftrc %[input_number], FPUL\n\t" // convert float to int. takes 3 cycles
    "fsca FPUL, DR0\n" // 3 cycle simultaneous sine/cosine
    : "=w" (__sine), "=f" (__cosine) // outputs
    : [input_number] "f" (input_float)  // inputs
    : "fpul" // clobbers
  );

  RETURN_FSCA_STRUCT output = {__sine, __cosine};
  return output;
}

// For float input, input_float is in degrees
static inline __attribute__((always_inline)) RETURN_FSCA_STRUCT MATH_fsca_Float_Deg(float input_float)
{
  input_float *= 182.044444444f;

  register float __sine __asm__("fr0");
  register float __cosine __asm__("fr1");

  asm volatile ("ftrc %[input_number], FPUL\n\t" // convert float to int. takes 3 cycles
    "fsca FPUL, DR0\n" // 3 cycle simultaneous sine/cosine
    : "=w" (__sine), "=f" (__cosine) // outputs
    : [input_number] "f" (input_float)  // inputs
    : "fpul" // clobbers
  );

  RETURN_FSCA_STRUCT output = {__sine, __cosine};
  return output;
}

// For float input, input_float is in radians
static inline __attribute__((always_inline)) RETURN_FSCA_STRUCT MATH_fsca_Float_Rad(float input_float)
{
  input_float *= 10430.3783505f;

  register float __sine __asm__("fr0");
  register float __cosine __asm__("fr1");

  asm volatile ("ftrc %[input_number], FPUL\n\t" // convert float to int. takes 3 cycles
    "fsca FPUL, DR0\n" // 3 cycle simultaneous sine/cosine
    : "=w" (__sine), "=f" (__cosine) // outputs
    : [input_number] "f" (input_float)  // inputs
    : "fpul" // clobbers
  );

  RETURN_FSCA_STRUCT output = {__sine, __cosine};
  return output;
}

//------------------------------------------------------------------------------
#else
//------------------------------------------------------------------------------
//
// This set of fsca functions is specifically for newer versions of GCC. They
// work fine under GCC 9.2.0.
//

//
// Integer input (faster)
//

// For int input, input_int is in native fsca units (fastest)
static inline __attribute__((always_inline)) _Complex float MATH_fsca_Int(unsigned int input_int)
{
  _Complex float output;

  asm volatile ("lds %[input_number], FPUL\n\t" // load int from register (1 cycle)
    "fsca FPUL, %[out]\n" // 3 cycle simultaneous sine/cosine
    : [out] "=d" (output) // outputs
    : [input_number] "r" (input_int)  // inputs
    : "fpul" // clobbers
  );

  return output;
}

// For int input, input_int is in degrees
static inline __attribute__((always_inline)) _Complex float MATH_fsca_Int_Deg(unsigned int input_int)
{
  // normalize whole number input degrees to fsca format
  input_int = ((1527099483ULL * input_int) >> 23);

  _Complex float output;

  asm volatile ("lds %[input_number], FPUL\n\t" // load int from register (1 cycle)
    "fsca FPUL, %[out]\n" // 3 cycle simultaneous sine/cosine
    : [out] "=d" (output) // outputs
    : [input_number] "r" (input_int)  // inputs
    : "fpul" // clobbers
  );

  return output;
}

// For int input, input_int is in radians
static inline __attribute__((always_inline)) _Complex float MATH_fsca_Int_Rad(unsigned int input_int)
{
  // normalize whole number input rads to fsca format
  input_int = ((2734261102ULL * input_int) >> 18);

  _Complex float output;

  asm volatile ("lds %[input_number], FPUL\n\t" // load int from register (1 cycle)
    "fsca FPUL, %[out]\n" // 3 cycle simultaneous sine/cosine
    : [out] "=d" (output) // outputs
    : [input_number] "r" (input_int)  // inputs
    : "fpul" // clobbers
  );

  return output;
}

//
// Float input (slower)
//

// For float input, input_float is in native fsca units
static inline __attribute__((always_inline)) _Complex float MATH_fsca_Float(float input_float)
{
  _Complex float output;

  asm volatile ("ftrc %[input_number], FPUL\n\t" // convert float to int. takes 3 cycles
    "fsca FPUL, %[out]\n" // 3 cycle simultaneous sine/cosine
    : [out] "=d" (output) // outputs
    : [input_number] "f" (input_float)  // inputs
    : "fpul" // clobbers
  );

  return output;
}

// For float input, input_float is in degrees
static inline __attribute__((always_inline)) _Complex float MATH_fsca_Float_Deg(float input_float)
{
  input_float *= 182.044444444f;

  _Complex float output;

  asm volatile ("ftrc %[input_number], FPUL\n\t" // convert float to int. takes 3 cycles
    "fsca FPUL, %[out]\n" // 3 cycle simultaneous sine/cosine
    : [out] "=d" (output) // outputs
    : [input_number] "f" (input_float)  // inputs
    : "fpul" // clobbers
  );

  return output;
}

// For float input, input_float is in radians
static inline __attribute__((always_inline)) _Complex float MATH_fsca_Float_Rad(float input_float)
{
  input_float *= 10430.3783505f;

  _Complex float output;

  asm volatile ("ftrc %[input_number], FPUL\n\t" // convert float to int. takes 3 cycles
    "fsca FPUL, %[out]\n" // 3 cycle simultaneous sine/cosine
    : [out] "=d" (output) // outputs
    : [input_number] "f" (input_float)  // inputs
    : "fpul" // clobbers
  );

  return output;
}

//------------------------------------------------------------------------------
#endif
//------------------------------------------------------------------------------

//==============================================================================
// Hardware vector and matrix operations
//==============================================================================
//
// These functions each have very specific usage instructions. Please be sure to
// read them before use or else they won't seem to work right!
//
// The following functions are available.
// Please see their definitions for important usage info, otherwise they may not
// work for you.
//
/*

  //------------------------------------------------------------------------------
  // Vector and matrix math operations
  //------------------------------------------------------------------------------

  // Inner/dot product (4x1 vec . 4x1 vec = scalar)
  float MATH_fipr(float x1, float x2, float x3, float x4, float y1, float y2, float y3, float y4)

  // Sum of Squares (w^2 + x^2 + y^2 + z^2)
  float MATH_Sum_of_Squares(float w, float x, float y, float z)

  // 4x4 Matrix transpose (XMTRX^T)
  void MATH_Matrix_Transpose(void)

  // 4x4 Matrix product (XMTRX and one from memory)
  void MATH_Matrix_Product(ALL_FLOATS_STRUCT * front_matrix)

  // 4x4 Matrix product (two from memory)
  void MATH_Load_Matrix_Product(ALL_FLOATS_STRUCT * matrix1, ALL_FLOATS_STRUCT * matrix2)

  //------------------------------------------------------------------------------
  // Matrix load and store operations
  //------------------------------------------------------------------------------

  // Load 4x4 XMTRX from memory
  void MATH_Load_XMTRX(ALL_FLOATS_STRUCT * back_matrix)

  // Store 4x4 XMTRX to memory
  ALL_FLOATS_STRUCT * MATH_Store_XMTRX(ALL_FLOATS_STRUCT * destination)
*/

//------------------------------------------------------------------------------
// Vector and matrix math operations
//------------------------------------------------------------------------------

// Inner/dot product: vec . vec = scalar
//                       _    _
//                      |  y1  |
//  [ x1 x2 x3 x4 ]  .  |  y2  | = scalar
//                      |  y3  |
//                      |_ y4 _|
//
// SH4 calling convention states we get 8 float arguments. Perfect!
static inline __attribute__((always_inline)) float MATH_fipr(float x1, float x2, float x3, float x4, float y1, float y2, float y3, float y4)
{
  // FR4-FR11 are the regs that are passed in, aka vectors FV4 and FV8.
  // Just need to make sure GCC doesn't modify anything, and these register vars do that job.

  // Temporary variables are necessary per GCC to avoid clobbering:
  // https://gcc.gnu.org/onlinedocs/gcc/Local-Register-Variables.html#Local-Register-Variables

  float tx1 = x1;
  float tx2 = x2;
  float tx3 = x3;
  float tx4 = x4;

  float ty1 = y1;
  float ty2 = y2;
  float ty3 = y3;
  float ty4 = y4;

  // vector FV4
  register float __x1 __asm__("fr4") = tx1;
  register float __x2 __asm__("fr5") = tx2;
  register float __x3 __asm__("fr6") = tx3;
  register float __x4 __asm__("fr7") = tx4;

  // vector FV8
  register float __y1 __asm__("fr8") = ty1;
  register float __y2 __asm__("fr9") = ty2;
  register float __y3 __asm__("fr10") = ty3;
  register float __y4 __asm__("fr11") = ty4;

  // take care of all the floats in one fell swoop
  asm volatile ("fipr FV4, FV8\n"
  : "+f" (__y4) // output (gets written to FR11)
  : "f" (__x1), "f" (__x2), "f" (__x3), "f" (__x4), "f" (__y1), "f" (__y2), "f" (__y3) // inputs
  : // clobbers
  );

  return __y4;
}

// Sum of Squares
//                   _   _
//                  |  w  |
//  [ w x y z ]  .  |  x  | = w^2 + x^2 + y^2 + z^2 = scalar
//                  |  y  |
//                  |_ z _|
//
static inline __attribute__((always_inline)) float MATH_Sum_of_Squares(float w, float x, float y, float z)
{
  // FR4-FR7 are the regs that are passed in, aka vector FV4.
  // Just need to make sure GCC doesn't modify anything, and these register vars do that job.

  // Temporary variables are necessary per GCC to avoid clobbering:
  // https://gcc.gnu.org/onlinedocs/gcc/Local-Register-Variables.html#Local-Register-Variables

  float tw = w;
  float tx = x;
  float ty = y;
  float tz = z;

  // vector FV4
  register float __w __asm__("fr4") = tw;
  register float __x __asm__("fr5") = tx;
  register float __y __asm__("fr6") = ty;
  register float __z __asm__("fr7") = tz;

  // take care of all the floats in one fell swoop
  asm volatile ("fipr FV4, FV4\n"
  : "+f" (__z) // output (gets written to FR7)
  : "f" (__w), "f" (__x), "f" (__y) // inputs
  : // clobbers
  );

  return __z;
}

// Matrix product: matrix * matrix = matrix
//
// These use the whole dang floating point unit.
//
//  [ ----------- ] [ ----------- ]     [ ----------- ]
//  [ ---Back---- ] [ ---Front--- ]  =  [ ---XMTRX--- ]
//  [ ---Matrix-- ] [ ---Matrix-- ]     [ ----------- ]
//  [ --(XMTRX)-- ] [ ----------- ]     [ ----------- ]
//
// Multiply a matrix stored in the back bank with a matrix loaded from memory
// Output is stored in the back bank (XMTRX)
static inline __attribute__((always_inline)) void MATH_Matrix_Product(ALL_FLOATS_STRUCT * front_matrix)
{
  /*
    // This prefetching should help a bit if placed suitably far enough in advance (not here)
    // Possibly right before this function call. Change the "front_matrix" variable appropriately.
    // SH4 does not support r/w or temporal prefetch hints, so we only need to pass in an address.
    __builtin_prefetch(front_matrix);
  */

  unsigned int prefetch_scratch;

  asm volatile (
    "mov %[fmtrx], %[pref_scratch]\n\t" // parallel-load address for prefetching (MT)
    "add #32, %[pref_scratch]\n\t" // offset by 32 (EX - flow dependency, but 'add' is actually parallelized since 'mov Rm, Rn' is 0-cycle)
    "fschg\n\t" // switch fmov to paired moves (FE)
    "pref @%[pref_scratch]\n\t" // Get a head start prefetching the second half of the 64-byte data (LS)
    // interleave loads and matrix multiply 4x4
    "fmov.d @%[fmtrx]+, DR0\n\t" // (LS)
    "fmov.d @%[fmtrx]+, DR2\n\t"
    "fmov.d @%[fmtrx]+, DR4\n\t" // (LS) want to issue the next one before 'ftrv' for parallel exec
    "ftrv XMTRX, FV0\n\t" // (FE)

    "fmov.d @%[fmtrx]+, DR6\n\t"
    "fmov.d @%[fmtrx]+, DR8\n\t" // prefetch should work for here
    "ftrv XMTRX, FV4\n\t"

    "fmov.d @%[fmtrx]+, DR10\n\t"
    "fmov.d @%[fmtrx]+, DR12\n\t"
    "ftrv XMTRX, FV8\n\t"

    "fmov.d @%[fmtrx], DR14\n\t" // (LS, but this will stall 'ftrv' for 3 cycles)
    "fschg\n\t" // switch back to single moves (and avoid stalling 'ftrv') (FE)
    "ftrv XMTRX, FV12\n\t" // (FE)
    // Save output in XF regs
    "frchg\n"
    : [fmtrx] "+r" ((unsigned int)front_matrix), [pref_scratch] "=&r" (prefetch_scratch) // outputs, "+" means r/w
    : // no inputs
    : "fr0", "fr1", "fr2", "fr3", "fr4", "fr5", "fr6", "fr7", "fr8", "fr9", "fr10", "fr11", "fr12", "fr13", "fr14", "fr15" // clobbers (GCC doesn't know about back bank, so writing to it isn't clobbered)
  );
}

// Load two 4x4 matrices and multiply them, storing the output into the back bank (XMTRX)
//
// MATH_Load_Matrix_Product() is slightly faster than doing this:
//    MATH_Load_XMTRX(matrix1)
//    MATH_Matrix_Product(matrix2)
// as it saves having to do 2 extraneous 'fschg' instructions.
//
static inline __attribute__((always_inline)) void MATH_Load_Matrix_Product(ALL_FLOATS_STRUCT * matrix1, ALL_FLOATS_STRUCT * matrix2)
{
  /*
    // This prefetching should help a bit if placed suitably far enough in advance (not here)
    // Possibly right before this function call. Change the "matrix1" variable appropriately.
    // SH4 does not support r/w or temporal prefetch hints, so we only need to pass in an address.
    __builtin_prefetch(matrix1);
  */

  unsigned int prefetch_scratch;

  asm volatile (
    "mov %[bmtrx], %[pref_scratch]\n\t" // (MT)
    "add #32, %[pref_scratch]\n\t" // offset by 32 (EX - flow dependency, but 'add' is actually parallelized since 'mov Rm, Rn' is 0-cycle)
    "fschg\n\t" // switch fmov to paired moves (note: only paired moves can access XDn regs) (FE)
    "pref @%[pref_scratch]\n\t" // Get a head start prefetching the second half of the 64-byte data (LS)
    // back matrix
    "fmov.d @%[bmtrx]+, XD0\n\t" // (LS)
    "fmov.d @%[bmtrx]+, XD2\n\t"
    "fmov.d @%[bmtrx]+, XD4\n\t"
    "fmov.d @%[bmtrx]+, XD6\n\t"
    "pref @%[fmtrx]\n\t" // prefetch fmtrx now while we wait (LS)
    "fmov.d @%[bmtrx]+, XD8\n\t" // bmtrx prefetch should work for here
    "fmov.d @%[bmtrx]+, XD10\n\t"
    "fmov.d @%[bmtrx]+, XD12\n\t"
    "mov %[fmtrx], %[pref_scratch]\n\t" // (MT)
    "add #32, %[pref_scratch]\n\t" // store offset by 32 in r0 (EX - flow dependency, but 'add' is actually parallelized since 'mov Rm, Rn' is 0-cycle)
    "fmov.d @%[bmtrx], XD14\n\t"
    "pref @%[pref_scratch]\n\t" // Get a head start prefetching the second half of the 64-byte data (LS)
    // front matrix
    // interleave loads and matrix multiply 4x4
    "fmov.d @%[fmtrx]+, DR0\n\t"
    "fmov.d @%[fmtrx]+, DR2\n\t"
    "fmov.d @%[fmtrx]+, DR4\n\t" // (LS) want to issue the next one before 'ftrv' for parallel exec
    "ftrv XMTRX, FV0\n\t" // (FE)

    "fmov.d @%[fmtrx]+, DR6\n\t"
    "fmov.d @%[fmtrx]+, DR8\n\t"
    "ftrv XMTRX, FV4\n\t"

    "fmov.d @%[fmtrx]+, DR10\n\t"
    "fmov.d @%[fmtrx]+, DR12\n\t"
    "ftrv XMTRX, FV8\n\t"

    "fmov.d @%[fmtrx], DR14\n\t" // (LS, but this will stall 'ftrv' for 3 cycles)
    "fschg\n\t" // switch back to single moves (and avoid stalling 'ftrv') (FE)
    "ftrv XMTRX, FV12\n\t" // (FE)
    // Save output in XF regs
    "frchg\n"
    : [bmtrx] "+&r" ((unsigned int)matrix1), [fmtrx] "+r" ((unsigned int)matrix2), [pref_scratch] "=&r" (prefetch_scratch) // outputs, "+" means r/w, "&" means it's written to before all inputs are consumed
    : // no inputs
    : "fr0", "fr1", "fr2", "fr3", "fr4", "fr5", "fr6", "fr7", "fr8", "fr9", "fr10", "fr11", "fr12", "fr13", "fr14", "fr15" // clobbers (GCC doesn't know about back bank, so writing to it isn't clobbered)
  );
}

//------------------------------------------------------------------------------
// Matrix load and store operations
//------------------------------------------------------------------------------

// Load a matrix from memory into the back bank (XMTRX)
static inline __attribute__((always_inline)) void MATH_Load_XMTRX(ALL_FLOATS_STRUCT * back_matrix)
{
  /*
    // This prefetching should help a bit if placed suitably far enough in advance (not here)
    // Possibly right before this function call. Change the "back_matrix" variable appropriately.
    // SH4 does not support r/w or temporal prefetch hints, so we only need to pass in an address.
    __builtin_prefetch(back_matrix);
  */

  unsigned int prefetch_scratch;

  asm volatile (
    "mov %[bmtrx], %[pref_scratch]\n\t" // (MT)
    "add #32, %[pref_scratch]\n\t" // offset by 32 (EX - flow dependency, but 'add' is actually parallelized since 'mov Rm, Rn' is 0-cycle)
    "fschg\n\t" // switch fmov to paired moves (note: only paired moves can access XDn regs) (FE)
    "pref @%[pref_scratch]\n\t" // Get a head start prefetching the second half of the 64-byte data (LS)
    "fmov.d @%[bmtrx]+, XD0\n\t"
    "fmov.d @%[bmtrx]+, XD2\n\t"
    "fmov.d @%[bmtrx]+, XD4\n\t"
    "fmov.d @%[bmtrx]+, XD6\n\t"
    "fmov.d @%[bmtrx]+, XD8\n\t"
    "fmov.d @%[bmtrx]+, XD10\n\t"
    "fmov.d @%[bmtrx]+, XD12\n\t"
    "fmov.d @%[bmtrx], XD14\n\t"
    "fschg\n" // switch back to single moves
    : [bmtrx] "+r" ((unsigned int)back_matrix), [pref_scratch] "=&r" (prefetch_scratch) // outputs, "+" means r/w
    : // no inputs
    : // clobbers (GCC doesn't know about back bank, so writing to it isn't clobbered)
  );
}

// Store XMTRX to memory
static inline __attribute__((always_inline)) ALL_FLOATS_STRUCT * MATH_Store_XMTRX(ALL_FLOATS_STRUCT * destination)
{
  /*
    // This prefetching should help a bit if placed suitably far enough in advance (not here)
    // Possibly right before this function call. Change the "destination" variable appropriately.
    // SH4 does not support r/w or temporal prefetch hints, so we only need to pass in an address.
    __builtin_prefetch( (ALL_FLOATS_STRUCT*)((unsigned char*)destination + 32) ); // Store works backwards, so note the '+32' here
  */

  char * output = ((char*)destination) + sizeof(ALL_FLOATS_STRUCT) + 8; // ALL_FLOATS_STRUCT should be 64 bytes

  asm volatile (
    "fschg\n\t" // switch fmov to paired moves (note: only paired moves can access XDn regs) (FE)
    "pref @%[dest_base]\n\t" // Get a head start prefetching the second half of the 64-byte data (LS)
    "fmov.d XD0, @-%[out_mtrx]\n\t" // These do *(--output) = XDn (LS)
    "fmov.d XD2, @-%[out_mtrx]\n\t"
    "fmov.d XD4, @-%[out_mtrx]\n\t"
    "fmov.d XD6, @-%[out_mtrx]\n\t"
    "fmov.d XD8, @-%[out_mtrx]\n\t"
    "fmov.d XD10, @-%[out_mtrx]\n\t"
    "fmov.d XD12, @-%[out_mtrx]\n\t"
    "fmov.d XD14, @-%[out_mtrx]\n\t"
    "fschg\n" // switch back to single moves
    : [out_mtrx] "+&r" ((unsigned int)output) // outputs, "+" means r/w, "&" means it's written to before all inputs are consumed
    : [dest_base] "r" ((unsigned int)destination) // inputs
    : "memory" // clobbers
  );

  return destination;
}


// In general, writing the entire required math routine in one asm function is
// the best way to go for performance reasons anyways, and in that situation one
// can just throw calling convention to the wind until returning back to C.

//==============================================================================
// Miscellaneous Functions
//==============================================================================
//
// The following functions are provided as examples of ways in which these math
// functions can be used.
//
// Reminder: 1 fsca unit = 1/182.044444444 of a degree or 1/10430.3783505 of a radian
// In order to make the best use of fsca units, a program must expect them from
// the outset and not "make them" by dividing radians or degrees to get them,
// otherwise it's just giving the 'fsca' instruction radians or degrees!
//
/*

  //------------------------------------------------------------------------------
  // Interpolation
  //------------------------------------------------------------------------------

  // Linear interpolation
  float MATH_Lerp(float a, float b, float t)
*/

//------------------------------------------------------------------------------
// Interpolation
//------------------------------------------------------------------------------

// Linear interpolation
static inline __attribute__((always_inline)) float MATH_Lerp(float a, float b, float t)
{
  return MATH_fmac(t, (b-a), a);
}

//==============================================================================
// Miscellaneous Snippets
//==============================================================================
//
// The following snippets are best implemented manually in user code (they can't
// be put into their own functions without incurring performance penalties).
//
// They also serve as examples of how one might use the functions in this header.
//
/*
  Normalize a vector (x, y, z) and get its pre-normalized magnitude (length)
*/

//
// Normalize a vector (x, y, z) and get its pre-normalized magnitude (length)
//
// magnitude = sqrt(x^2 + y^2 + z^2)
// (x, y, z) = 1/magnitude * (x, y, z)
//
// x, y, z, and magnitude are assumed already existing floats
//

/* -- start --

  // Don't need an 'else' with this (if length is 0, x = y = z = 0)
  magnitude = 0;

  if(__builtin_expect(x || y || z, 1))
  {
    temp = MATH_Sum_of_Squares(x, y, z, 0); // temp = x^2 + y^2 + z^2 + 0^2
    float normalizer = MATH_fsrra(temp); // 1/sqrt(temp)
    x = normalizer * x;
    y = normalizer * y;
    z = normalizer * z;
    magnitude = MATH_Fast_Invert(normalizer);
  }

-- end -- */


#endif /* __SH4_MATH_H_ */

