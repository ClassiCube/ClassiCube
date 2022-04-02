#ifndef CC_LBACKEND_H
#define CC_LBACKEND_H
#include "Core.h"
/* Implements the gui drawing backend for the Launcher
	Copyright 2014-2021 ClassiCube | Licensed under BSD-3
*/
struct Bitmap;
struct LWidget;
struct LButton;
struct LCheckbox;
struct LInput;
struct LLabel;
struct LLine;
struct LSlider;

void LBackend_CalcOffsets(void);
void LBackend_WidgetRepositioned(struct LWidget* w);

void LBackend_InitButton(struct LButton* w, int width, int height);
void LBackend_UpdateButton(struct LButton* w);
void LBackend_DrawButton(struct LButton* w);

void LBackend_InitCheckbox(struct LCheckbox* w);
void LBackend_DrawCheckbox(struct LCheckbox* w);

void LBackend_InitInput(struct LInput* w, int width);
void LBackend_DrawInput(struct LInput* w, const cc_string* text);

void LBackend_InitLabel(struct LLabel* w);
void LBackend_UpdateLabel(struct LLabel* w);
void LBackend_DrawLabel(struct LLabel* w);

void LBackend_InitLine(struct LLine* w, int width);
void LBackend_DrawLine(struct LLine* w);

void LBackend_InitSlider(struct LSlider* w, int width, int height);
void LBackend_DrawSlider(struct LSlider* w);
#endif
