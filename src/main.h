#ifndef CC_MAIN_H
#define CC_MAIN_H
#include "String.h"
/* Utility constants and methods for command line arguments
   Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/
CC_BEGIN_HEADER

#define DEFAULT_SINGLEPLAYER_ARG "--singleplayer"
#define DEFAULT_RESUME_ARG       "--resume"

struct ResumeInfo {
	cc_string user, ip, port, server, mppass;
	char _userBuffer[STRING_SIZE], _serverBuffer[STRING_SIZE];
	char _ipBuffer[16], _portBuffer[16], _mppassBuffer[STRING_SIZE];
};

cc_bool Resume_Parse(struct ResumeInfo* info, cc_bool full);

cc_bool DirectUrl_Claims(const cc_string* STRING_REF input, cc_string* addr, cc_string* user, cc_string* mppass);
void    DirectUrl_ExtractAddress(const cc_string* STRING_REF addr, cc_string* ip, cc_string* port);

CC_END_HEADER
#endif
