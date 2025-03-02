#include "Processes.r"
#include "Retro68.r"

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
	8192 * 1024, // preferred (maximum) memory size
	2900 * 1024, // minimum memory size (enough to load launcher)
};
