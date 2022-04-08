#ifndef CC_LBACKEND_H
#define CC_LBACKEND_H
#include "Core.h"
/* Implements the gui drawing backend for the Launcher
	Copyright 2014-2021 ClassiCube | Licensed under BSD-3
*/
struct Bitmap;
struct LScreen;
struct LWidget;
struct LButton;
struct LCheckbox;
struct LInput;
struct LLabel;
struct LLine;
struct LSlider;
struct LTable;

void LBackend_Init(void);
void LBackend_WidgetRepositioned(struct LWidget* w);
void LBackend_SetScreen(struct LScreen* s);
void LBackend_CloseScreen(struct LScreen* s);

void LBackend_ButtonInit(struct LButton* w, int width, int height);
void LBackend_ButtonUpdate(struct LButton* w);
void LBackend_ButtonDraw(struct LButton* w);

void LBackend_CheckboxInit(struct LCheckbox* w);
void LBackend_CheckboxDraw(struct LCheckbox* w);

void LBackend_InputInit(struct LInput* w, int width);
void LBackend_InputUpdate(struct LInput* w);
void LBackend_InputDraw(struct LInput* w);

void LBackend_InputTick(struct LInput* w);
void LBackend_InputSelect(struct LInput* w, int idx, cc_bool wasSelected);
void LBackend_InputUnselect(struct LInput* w);

void LBackend_LabelInit(struct LLabel* w);
void LBackend_LabelUpdate(struct LLabel* w);
void LBackend_LabelDraw(struct LLabel* w);

void LBackend_LineInit(struct LLine* w, int width);
void LBackend_LineDraw(struct LLine* w);

void LBackend_SliderInit(struct LSlider* w, int width, int height);
void LBackend_SliderUpdate(struct LSlider* w);
void LBackend_SliderDraw(struct LSlider* w);

void LBackend_TableDraw(struct LTable* w);
void LBackend_TableReposition(struct LTable* w);

void LBackend_TableMouseDown(struct LTable* w, int idx);
void LBackend_TableMouseUp(struct   LTable* w, int idx);
void LBackend_TableMouseMove(struct LTable* w, int idx);
#endif
