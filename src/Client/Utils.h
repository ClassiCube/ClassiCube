#ifndef CC_UTILS_H
#define CC_UTILS_H
#include "Typedefs.h"
#include "String.h"
/* Implements various utility functions.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

Int32 Utils_AccumulateWheelDelta(Real32* accmulator, Real32 delta);

UInt32 Utils_ParseEnum(STRING_PURE String* text, UInt32 defValue, const UInt8** names, UInt32 namesCount);

bool Utils_IsValidInputChar(UInt8 c, bool supportsCP437);

#define Utils_AdjViewDist(value) ((Int32)(1.4142135f * (value)))
#endif