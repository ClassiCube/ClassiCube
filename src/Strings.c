#include "Constants.h"
#include "Platform.h"
#include "Server.h"
#include "String.h"
#include "Strings.h"

unsigned char CC_CurrentLanguage = 0;
cc_string gameName;

static char appBuffer[STRING_SIZE];
void applyLanguageToGame() {
	String_InitArray(Server.AppName, appBuffer);
    String_InitArray(gameName, appBuffer);
	String_AppendConst(&Server.AppName, ccStrings_GameTitle[CC_CurrentLanguage]);
	String_AppendConst(&Server.AppName, " ");
	String_AppendConst(&Server.AppName, GAME_APP_VER);
	String_AppendConst(&Server.AppName, Platform_AppNameSuffix);
    gameName = Server.AppName;
}