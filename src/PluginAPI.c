#include "Core.h"

#ifdef CC_BUILD_PLUGINS
#include "Game.h"
#include "Utils.h"
#include "Logger.h"
#include "String_.h"
#include "Chat.h"
#include "Platform.h"
#define INTERNAL_PLUGIN_HDR
#include "PluginAPI.h"

static void LoadPlugin(const cc_string* path, void* obj, int isDirectory) {
	void* lib;
	void* verSym;  /* EXPORT int Plugin_ApiVersion = GAME_API_VER; */
	void* compSym; /* EXPORT struct IGameComponent Plugin_Component = { (whatever) } */
	int ver;
	if (isDirectory) return;

	/* ignore accepted.txt, deskop.ini, .pdb files, etc */
	if (!String_CaselessEnds(path, &DynamicLib_Ext)) return;
	/* don't try to load 32 bit plugins on 64 bit OS or vice versa */
	if (sizeof(void*) == 4 && String_ContainsConst(path, "_64.")) return;
	if (sizeof(void*) == 8 && String_ContainsConst(path, "_32.")) return;

	lib = DynamicLib_Load2(path);
	if (!lib) { Logger_DynamicLibWarn("loading", path); return; }

	verSym  = DynamicLib_Get2(lib, "Plugin_ApiVersion");
	if (!verSym)  { Logger_DynamicLibWarn("getting version of", path); return; }
	compSym = DynamicLib_Get2(lib, "Plugin_Component");
	if (!compSym) { Logger_DynamicLibWarn("initing", path); return; }

	ver = *((int*)verSym);
	if (ver < GAME_API_VER) {
		Chat_Add1("&c%s plugin is outdated! Try getting a more recent version.", path);
		return;
	} else if (ver > GAME_API_VER) {
		Chat_Add1("&cYour game is too outdated to use %s plugin! Try updating it.", path);
		return;
	}

	Game_AddComponent((struct IGameComponent*)compSym);
}

void Plugins_LoadAll(void) {
	static const cc_string dir = String_FromConst("plugins");
	cc_result res;

	Utils_EnsureDirectory("plugins");
	res = Directory_Enum(&dir, NULL, LoadPlugin);
	if (res) Logger_SysWarn(res, "enumerating plugins directory");
}
#else
void Plugins_LoadAll(void) { }
#endif

