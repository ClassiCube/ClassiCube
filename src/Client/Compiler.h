#ifndef CS_COMPILER_H
#define CS_COMPILER_H
#include "Typedefs.h"
/* Compiler-specific attributes and helpers
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/


// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the CLIENT_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// CLIENT_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.

#ifdef CLIENT_EXPORTS
#define CLIENT_FUNC __declspec(dllexport)
#else
#define CLIENT_FUNC __declspec(dllimport)
#endif

#define EXPORT_FUNC __declspec(dllexport)
#define IMPORT_FUNC __declspec(dllimport)

int __stdcall DllMain(void* hinstDLL, UInt32 fdwReason, void* pvReserved);
#endif