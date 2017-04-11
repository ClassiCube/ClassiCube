#ifndef CS_COMPILER_H
#define CS_COMPILER_H

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the CLIENT_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// CLIENT_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.

#ifdef CLIENT_EXPORTS
#define CLIENT_API __declspec(dllexport)
#else
#define CLIENT_API __declspec(dllimport)
#endif

extern CLIENT_API int nClient;

/*

#include "Client.h"

// This is an example of an exported variable
CLIENT_API int nClient=0;

// This is an example of an exported function.
CLIENT_API int fnClient(void)
{
return 42;
}

*/

#endif