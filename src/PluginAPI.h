#ifndef CC_PLUGIN_H
#define CC_PLUGIN_H
#include "PluginAPI.h"
CC_BEGIN_HEADER

/* Represents the interface for plugins.
   Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/

#if !defined INTERNAL_PLUGIN_HDR && defined CC_BUILD_WIN
	// When compiling external plugins, functions/variables need to be imported from ClassiCube exe instead of exporting them
	// need to specifically declare as imported for MSVC
	#define CC_API __declspec(dllimport)
	#define CC_VAR __declspec(dllimport)

	// special attribute to export symbols with MSVC/MinGW
	#define PLUGIN_EXPORT __declspec(dllexport)
#elif !defined INTERNAL_PLUGIN_HDR
	// // When compiling external plugins, functions/variables need to be imported from ClassiCube exe instead of exporting them
	#define CC_API
	#define CC_VAR

	// public symbols are usually exported when compiling a shared library with GCC
	// but just to be on the safe side, ensure that it's always exported
	#define PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

/* All plugins are required to have the following */
/*   PLUGIN_EXPORT int Plugin_ApiVersion = GAME_API_VER; */
/*   PLUGIN_EXPORT struct IGameComponent Plugin_Component = { (whatever) } */
#define GAME_API_VER 1


struct PluginInterface {
	int   version;
	int   type;
	void* value;
};
#define IFACE_TYPE_CHAT 1


struct ChatInterface {
	void (*Send) (const cc_string* text, cc_bool logUsage);
	void (*Add)  (const cc_string* text);
	void (*AddOf)(const cc_string* text, int msgType);

	void (*AddRaw)(const char* raw);
	void (*Add1)  (const char* format, const void* a1);
	void (*Add2)  (const char* format, const void* a1, const void* a2);
	void (*Add3)  (const char* format, const void* a1, const void* a2, const void* a3);
	void (*Add4)  (const char* format, const void* a1, const void* a2, const void* a3, const void* a4);
};
#define PLUGIN_INTERFACE_CHAT(value) { IFACE_TYPE_CHAT, 1, value }

CC_END_HEADER

