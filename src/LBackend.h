#ifndef CC_LBACKEND_H
#define CC_LBACKEND_H
#include "Core.h"
/* Implements the gui drawing backend for the Launcher
	Copyright 2014-2021 ClassiCube | Licensed under BSD-3
*/
struct Bitmap;
struct LButton;
struct LInput;
struct LLabel;
struct LLine;
struct LSlider;

/* Whether tiles have been extracted from a terrain atlas */
cc_bool LBackend_HasTextures(void);
/* Extracts tiles from the given terrain atlas bitmap */
void LBackend_LoadTextures(struct Bitmap* bmp);
/* Redraws the specified region with the background pixels */
void LBackend_ResetArea(int x, int y, int width, int height);
/* Redraws all pixels with default background */
void LBackend_ResetPixels(void);

void LBackend_CalcOffsets(void);
void LBackend_DrawButton(struct LButton* w);
void LBackend_DrawInput(struct LInput* w, const cc_string* text);
void LBackend_DrawLabel(struct LLabel* w);
void LBackend_DrawLine(struct LLine* w);
void LBackend_DrawSlider(struct LSlider* w);
#endif
