#ifndef CS_OPTIONS_H
#define CS_OPTIONS_H
#include "Typedefs.h"
/* Manages loading and saving options.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/


/* Sides of a block. Constants defined to match blockface field in CPE PlayerClicked. */
typedef UInt8 FpsLimitMethod;
#define FpsLimitMethod_VSync  0
#define FpsLimitMethod_30FPS  1
#define FpsLimitMethod_60FPS  2
#define FpsLimitMethod_120FPS 3
#define FpsLimitMethod_None   4
#endif