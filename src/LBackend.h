#ifndef CC_LBACKEND_H
#define CC_LBACKEND_H
#include "Core.h"
/* Implements the gui drawing backend for the Launcher
	Copyright 2014-2021 ClassiCube | Licensed under BSD-3
*/
struct Bitmap;
struct LButton;
struct LCheckbox;
struct LInput;
struct LLabel;
struct LLine;
struct LSlider;

#define CB_SIZE  24
#define CB_OFFSET 8

void LBackend_CalcOffsets(void);
void LBackend_DrawButton(struct LButton* w);
void LBackend_DrawCheckbox(struct LCheckbox* w);
void LBackend_DrawInput(struct LInput* w, const cc_string* text);
void LBackend_DrawLabel(struct LLabel* w);
void LBackend_DrawLine(struct LLine* w);
void LBackend_DrawSlider(struct LSlider* w);
#endif
