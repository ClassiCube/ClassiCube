#ifndef CS_RANDOM_H
#define CS_RANDOM_H
#include "Typedefs.h"

// Based on https://docs.oracle.com/javase/7/docs/api/java/util/Random.html
typedef Int64 Random;

void Random_Init(Random* rnd, Int32 seed);
Int32 Random_NextRange(Random* rnd, Int32 min, Int32 max);
Int32 Random_Next(Random* rnd, Int32 n);
Real32 Random_NextFloat(Random* rnd);
#endif