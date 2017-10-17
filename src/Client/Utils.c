#include "Utils.h"

Int32 Utils_AccumulateWheelDelta(Real32* accmulator, Real32 delta) {
	/* Some mice may use deltas of say (0.2, 0.2, 0.2, 0.2, 0.2) */
	/* We must use rounding at final step, not at every intermediate step. */
	*accmulator += delta;
	Int32 steps = (Int32)*accmulator;
	*accmulator -= steps;
	return steps;
}

UInt32 Utils_ParseEnum(STRING_TRANSIENT String* text, UInt32 defValue, const UInt8** names, UInt32 namesCount) {
	UInt32 i;
	for (i = 0; i < namesCount; i++) {
		String name = String_FromReadonly(names[i]);
		if (String_CaselessEquals(text, &name)) return i;
	}
	return defValue;
}