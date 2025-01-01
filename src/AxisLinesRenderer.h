#ifndef CC_AXISLINESRENDERER_H
#define CC_AXISLINESRENDERER_H
#include "Core.h"
/* Renders 3 lines showing direction of each axis.
   Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/
CC_BEGIN_HEADER

struct IGameComponent;
extern struct IGameComponent AxisLinesRenderer_Component;
/* Whether the 3 axis lines should be rendered */
extern cc_bool AxisLinesRenderer_Enabled;

void AxisLinesRenderer_Render(void);

CC_END_HEADER
#endif
