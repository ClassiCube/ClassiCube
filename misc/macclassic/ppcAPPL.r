#include "Processes.r"
#include "CodeFragments.r"

resource 'cfrg' (0) {
	{
		kPowerPCCFragArch, kIsCompleteCFrag, kNoVersionNum, kNoVersionNum,
		kDefaultStackSize, kNoAppSubFolder,
		kApplicationCFrag, kDataForkCFragLocator, kZeroOffset, kCFragGoesToEOF,
		"ClassiCube"
	}
};

resource 'SIZE' (-1) {
	reserved,
	ignoreSuspendResumeEvents,
	reserved,
	cannotBackground,
	needsActivateOnFGSwitch,
	backgroundAndForeground,
	dontGetFrontClicks,
	ignoreChildDiedEvents,
	is32BitCompatible,
	notHighLevelEventAware,
	onlyLocalHLEvents,
	notStationeryAware,
	dontUseTextEditServices,
	reserved,
	reserved,
	reserved,
	3072 * 1024,
	8192 * 1024
};
