#include "Args.h"
#include "Platform.h"
#include "Constants.h"

/**
 * Matches up to the ArgsType enum defined in Args.h
 * When modifying this, don't forget to add the new entry
 * to the enum
 */
static const char *Args_ArgPrefixes[ARGS_TYPE_COUNT] = {
		"-u", // username
		"-s", // secret (Password)
		"-a", // address (IP Address)
		"-p", // port
		"-h", // hash
		"-d"  // directory
};

static String Args_ArgCache[ARGS_TYPE_COUNT];
static int Args_Count = 0;
static String Args_Raw[GAME_MAX_CMDARGS];

void Args_Parse(int count, const String args[GAME_MAX_CMDARGS]) {
	int i, j;
	// WIth the new system, every argument must have an identifier

	for (i = 0; i < GAME_MAX_CMDARGS; ++i) {
		Args_Raw[i] = args[i];

	}
	Args_Count = count / 2;

	for (i = 0; i < ARGS_TYPE_COUNT; ++i) {
		Args_ArgCache[i] = String_Empty;
	}

	for (i = 0; i < count; ++i) {
		for (j = 0; j < ARGS_TYPE_COUNT; ++j) {
			if (String_CaselessEqualsConst(&args[i], Args_ArgPrefixes[j])) {
				Args_ArgCache[j] = args[++i];
				break;
			}
		}
	}
}

int Args_Init(int argc, char **argv) {
	int count;
	String args[GAME_MAX_CMDARGS];

	count = Platform_GetCommandLineArgs(argc, argv, args);

	if (count % 2 != 0) {
		return -1;
	}

	Args_Parse(count, args);

	return Args_GetArgsCount();
}

const String *Args_GetUsername(void) {
	return &Args_ArgCache[ARGS_TYPE_USERNAME];
}

const String *Args_GetMPPass(void) {
	return &Args_ArgCache[ARGS_TYPE_MPPASS];
}

const String *Args_GetIP(void) {
	return &Args_ArgCache[ARGS_TYPE_IP];
}

const String *Args_GetPort(void) {
	return &Args_ArgCache[ARGS_TYPE_PORT];
}

const String *Args_GetHash(void) {
	return &Args_ArgCache[ARGS_TYPE_HASH];
}

const String *Args_GetWorkingDir(void) {
	return &Args_ArgCache[ARGS_TYPE_WORKING_DIR];
}

int Args_GetArgsCount(void) {
	return Args_Count;
}

cc_bool Args_OnlyHasHash(void) {
	return Args_GetArgsCount() < 3 && Args_GetHash()->buffer != NULL;
}

cc_bool Args_OnlyHasUsername(void) {
	return Args_GetArgsCount() < 3 && Args_GetUsername()->buffer != NULL;
}

cc_bool Args_HasRunningArgs(void) {
	return Args_GetUsername()->buffer != NULL &&
			Args_GetMPPass()->buffer != NULL &&
			Args_GetIP()->buffer != NULL &&
			Args_GetPort()->buffer != NULL;
}

cc_bool Args_HasNoArguments(void) {
	return Args_GetArgsCount() == 0 ||
		   (Args_GetWorkingDir()->buffer != NULL && Args_GetArgsCount() == 1);
}

void Args_SetCustomArgs(const String *raw) {
	int count;
	String args[GAME_MAX_CMDARGS];
	count = String_UNSAFE_Split(raw, ' ', args, 4);
	Args_Parse(count, args);
}

const String *Args_GetRawArgs(void) {
	return Args_Raw;
}

cc_bool Args_RequestedSpecificDirectory(void) {
	return Args_GetWorkingDir()->buffer != NULL;
}

cc_bool Args_HasEnoughArgs(void) {
	return Args_OnlyHasHash() || Args_OnlyHasUsername() || Args_HasRunningArgs() || Args_HasNoArguments();
}

