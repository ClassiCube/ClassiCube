#ifndef CC_LBACKEND_H
#define CC_LBACKEND_H
#include "Core.h"
CC_BEGIN_HEADER

/* 
Abstracts the gui drawing backend for the Launcher
Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/
struct Context2D;
struct LScreen;
struct LWidget;
struct LButton;
struct LCheckbox;
struct LInput;
struct LLabel;
struct LLine;
struct LSlider;
struct LTable;
struct Flag;

typedef void (*LBackend_DrawHook)(struct Context2D* ctx);
extern LBackend_DrawHook LBackend_Hooks[4];

void LBackend_Init(void);
void LBackend_Free(void);
void LBackend_SetScreen(struct LScreen* s);
void LBackend_CloseScreen(struct LScreen* s);

void LBackend_UpdateTitleFont(void);
void LBackend_DrawTitle(struct Context2D* ctx, const char* title);

void LBackend_DecodeFlag(struct Flag* flag, cc_uint8* data, cc_uint32 len);
void LBackend_TableFlagAdded(struct LTable* w);

/* Marks the entire launcher contents as needing to be redrawn */
void LBackend_Redraw(void);
/* Marks the given widget as needing to be redrawn */
void LBackend_NeedsRedraw(void* widget);
/* Marks the entire window as needing to be redrawn */
void LBackend_MarkAllDirty(void);
/* Marks the given area/region as needing to be redrawn */
void LBackend_MarkAreaDirty(int x, int y, int width, int height);

void LBackend_ThemeChanged(void);
void LBackend_Tick(void);
void LBackend_AddDirtyFrames(int frames);
void LBackend_LayoutWidget(struct LWidget* w);

void LBackend_InitFramebuffer(void);
void LBackend_FreeFramebuffer(void);

void LBackend_ButtonInit(struct LButton* w, int width, int height);
void LBackend_ButtonUpdate(struct LButton* w);
void LBackend_ButtonDraw(struct LButton* w);

void LBackend_CheckboxInit(struct LCheckbox* w);
void LBackend_CheckboxUpdate(struct LCheckbox* w);
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

void LBackend_TableInit(struct LTable* w);
void LBackend_TableUpdate(struct LTable* w);
/* Adjusts Y position of rows and number of visible rows */
void LBackend_TableReposition(struct LTable* w);
void LBackend_TableDraw(struct LTable* w);

void LBackend_TableMouseDown(struct LTable* w, int idx);
void LBackend_TableMouseUp(struct   LTable* w, int idx);
void LBackend_TableMouseMove(struct LTable* w, int idx);

CC_END_HEADER
#endif
