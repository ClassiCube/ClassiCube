#ifndef CS_UTILS_H
#define CS_UTILS_H
#include "Typedefs.h"
#include "String.h"

Int32 Utils_AccumulateWheelDelta(Real32* accmulator, Real32 delta);

UInt32 Utils_ParseEnum(STRING_TRANSIENT String* text, UInt32 defValue, const UInt8** names, UInt32 namesCount);

#define Utils_AdjViewDist(value) ((Int32)(1.4142135f * (value)))
#endif