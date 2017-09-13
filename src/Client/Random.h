#ifndef CS_RANDOM_H
#define CS_RANDOM_H
#include "Typedefs.h"
/* Implemented a random number generation algorithm.
   Based on Java's implementation from https://docs.oracle.com/javase/7/docs/api/java/util/Random.html
   Copyright 2014 - 2017 ClassicalSharp | Licensed under BSD-3
*/

typedef Int64 Random;
/* Initalises the random number generator with an initial given seed. */
void Random_Init(Random* rnd, Int32 seed);
/* Initalises the random number generator with an initial see based on current time. */
void Random_InitFromCurrentTime(Random* rnd);
/* Returns a random number between min inclusive and max exclusive. */
Int32 Random_Range(Random* rnd, Int32 min, Int32 max);
/* Returns a random number from 0 inclusive to n exlucisve. */
Int32 Random_Next(Random* rnd, Int32 n);
/* Returns a random number between 0 inclusive and 1 exlusive. */
Real32 Random_Float(Random* rnd);
#endif