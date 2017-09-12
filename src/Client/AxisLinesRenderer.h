#ifndef CS_AXISLINESRENDERER_H
#define CS_AXISLINESRENDERER_H
#include "Typedefs.h"
#include "GameStructs.h"
/* Renders 3 lines showing direction of each axis.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Creates game component implementation. */
IGameComponent AxisLinesRenderer_MakeGameComponent(void);
/* Renders axis lines, if ShowAxisLines is enabled. */
void AxisLinesRenderer_Render(Real64 delta);
#endif