#ifndef CC_ARGS_H
#define CC_ARGS_H

#include "Core.h"
#include "String.h"

/* Parses and handles arguments
   Copyright 2014-2019 ClassiCube | Licensed under BSD-3
*/

enum ArgsType {
    ARGS_TYPE_USERNAME,
    ARGS_TYPE_MPPASS,
    ARGS_TYPE_IP,
    ARGS_TYPE_PORT,
    ARGS_TYPE_HASH,
    ARGS_TYPE_WORKING_DIR,
    ARGS_TYPE_COUNT
};


/* Returns number of actual arguments not counting identifiers */
int Args_Init(int argc, char **argv);

void Args_SetCustomArgs(const String *raw);

const String *Args_GetUsername(void);

const String *Args_GetMPPass(void);

const String *Args_GetIP(void);

const String *Args_GetPort(void);

const String *Args_GetHash(void);

const String *Args_GetWorkingDir(void);

int Args_GetArgsCount(void);

const String *Args_GetRawArgs(void);

cc_bool Args_OnlyHasHash(void);

cc_bool Args_OnlyHasUsername(void);

cc_bool Args_HasRunningArgs(void);

cc_bool Args_HasNoArguments(void);

cc_bool Args_RequestedSpecificDirectory(void);

cc_bool Args_HasEnoughArgs(void);

#endif //CC_ARGS_H
