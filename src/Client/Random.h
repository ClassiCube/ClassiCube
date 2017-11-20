#ifndef CC_RANDOM_H
#define CC_RANDOM_H
#include "Typedefs.h"
/* Implemented a random number generation algorithm.
   Based on Java's implementation from https://docs.oracle.com/javase/7/docs/api/java/util/Random.html
   Copyright 2014 - 2017 ClassicalSharp | Licensed under BSD-3
*/

typedef Int64 Random;
void Random_Init(Random* rnd, Int32 seed);
void Random_InitFromCurrentTime(Random* rnd);
void Random_SetSeed(Random* rnd, Int32 seed);
/* Returns integer from min inclusive to max exclusive */
Int32 Random_Range(Random* rnd, Int32 min, Int32 max);
/* Returns integer from 0 inclusive to n exclusive */
Int32 Random_Next(Random* rnd, Int32 n);
/* Returns real from 0 inclusive to 1 exclusive */
Real32 Random_Float(Random* rnd);
#endif