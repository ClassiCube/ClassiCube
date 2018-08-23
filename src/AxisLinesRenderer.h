#ifndef CC_AXISLINESRENDERER_H
#define CC_AXISLINESRENDERER_H
#include "Core.h"
/* Renders 3 lines showing direction of each axis.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

struct IGameComponent;
void AxisLinesRenderer_MakeComponent(struct IGameComponent* comp);
void AxisLinesRenderer_Render(Real64 delta);
#endif
