#ifndef CC_AXISLINESRENDERER_H
#define CC_AXISLINESRENDERER_H
#include "Core.h"
/* Renders 3 lines showing direction of each axis.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

struct IGameComponent;
extern struct IGameComponent AxisLinesRenderer_Component;

void AxisLinesRenderer_Render(double delta);
#endif
