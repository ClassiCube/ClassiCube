#ifndef CS_TYPEDEFS_H
#define CS_TYPEDEFS_H
/* Ensures variables are of a fixed size.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/


typedef unsigned __int8  UInt8;
typedef unsigned __int16 UInt16;
typedef unsigned __int32 UInt32;
typedef unsigned __int64 UInt64;

typedef signed __int8  Int8;
typedef signed __int16 Int16;
typedef signed __int32 Int32;
typedef signed __int64 Int64;

typedef float  Real32;
typedef double Real64;

typedef UInt8 bool;
#define true 1
#define false 0
#define NULL 0

typedef UInt8 BlockID;
typedef UInt8 EntityID;

#endif