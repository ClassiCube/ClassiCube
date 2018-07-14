#ifndef CC_AXISLINESRENDERER_H
#define CC_AXISLINESRENDERER_H
#include "GameStructs.h"
/* Renders 3 lines showing direction of each axis.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

IGameComponent AxisLinesRenderer_MakeComponent(void);
void AxisLinesRenderer_Render(Real64 delta);
#endif
