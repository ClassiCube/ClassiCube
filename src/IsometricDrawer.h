#ifndef CC_ISOMETRICDRAWER_H
#define CC_ISOMETRICDRAWER_H
#include "Core.h"
/* Draws 2D isometric blocks for the hotbar and inventory UIs.
   Copyright 2014-2023 ClassiCube | Licensed under BSD-3
*/
struct VertexTextured;

/* Maximum number of vertices used to draw a block in isometric way. */
#define ISOMETRICDRAWER_MAXVERTICES 12

/* Sets up state to begin drawing blocks isometrically */
void IsometricDrawer_BeginBatch(struct VertexTextured* vertices, int* state);
/* Buffers the vertices needed to draw the given block at the given position */
void IsometricDrawer_AddBatch(BlockID block, float size, float x, float y);
/* Returns the number of buffered vertices */
int  IsometricDrawer_EndBatch(void);
/* Draws the buffered vertices */
void IsometricDrawer_Render(int count, int offset, int* state);
#endif
