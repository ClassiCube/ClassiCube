cc_uint8 Platform_Flags = PLAT_FLAG_SINGLE_PROCESS | PLAT_FLAG_APP_EXIT;

/*########################################################################################################################*
*-----------------------------------------------------Main entrypoint-----------------------------------------------------*
*#########################################################################################################################*/
#include "main_impl.h"

int main(int argc, char** argv) {
	SetupProgram(argc, argv);
	while (Window_Main.Exists) { 
		RunProgram(argc, argv);
	}
	
	Window_Free();
	return 0;
}


/*########################################################################################################################*
*-----------------------------------------------------Process/Module------------------------------------------------------*
*#########################################################################################################################*/
cc_result Process_StartGame2(const cc_string* args, int numArgs) {
	Platform_LogConst("START CLASSICUBE");
	return SetGameArgs(args, numArgs);
}

int Platform_GetCommandLineArgs(int argc, STRING_REF char** argv, cc_string* args) {
	if (gameHasArgs) return GetGameArgs(args);
	// Consoles *sometimes* doesn't use argv[0] for program name and so argc will be 0
	//  (e.g. when running via some emulators)
	if (!argc) return 0;

#if defined CC_BUILD_PS1 || defined CC_BUILD_SATURN
	// When running in DuckStation at least, argv was a five element array of empty strings ???
	return 0;
#else
	
	argc--; argv++; // skip executable path argument

	int count = min(argc, GAME_MAX_CMDARGS);
	Platform_Log1("ARGS: %i", &count);
	
	for (int i = 0; i < count; i++) 
	{
		args[i] = String_FromReadonly(argv[i]);
		Platform_Log2("  ARG %i = %c", &i, argv[i]);
	}
	return count;
#endif
}

cc_result Platform_SetDefaultCurrentDirectory(int argc, char **argv) {
	return 0;
}

