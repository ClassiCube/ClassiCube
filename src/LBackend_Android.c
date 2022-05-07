#include "LBackend.h"
#if defined CC_BUILD_ANDROID
#include "Launcher.h"
#include "Drawer2D.h"
#include "Window.h"
#include "LWidgets.h"
#include "String.h"
#include "Gui.h"
#include "Drawer2D.h"
#include "Launcher.h"
#include "ExtMath.h"
#include "Window.h"
#include "Funcs.h"
#include "LWeb.h"
#include "Platform.h"
#include "LScreens.h"
#include "Input.h"
#include "Utils.h"
#include "Event.h"
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/keycodes.h>
#include <android/bitmap.h>

struct FontDesc titleFont, textFont, hintFont, logoFont, rowFont;

static int xBorder, xBorder2, xBorder3, xBorder4;
static int yBorder, yBorder2, yBorder3, yBorder4;
static int xInputOffset, yInputOffset, inputExpand;
static int caretOffset, caretWidth, caretHeight;
static int scrollbarWidth, dragPad, gridlineWidth, gridlineHeight;
static int hdrYOffset, hdrYPadding, rowYOffset, rowYPadding;
static int cellXOffset, cellXPadding, cellMinWidth;
static int flagXOffset, flagYOffset;

static void HookEvents(void);
static void LBackend_InitHooks(void);
void LBackend_Init(void) {
    xBorder = Display_ScaleX(1); xBorder2 = xBorder * 2; xBorder3 = xBorder * 3; xBorder4 = xBorder * 4;
    yBorder = Display_ScaleY(1); yBorder2 = yBorder * 2; yBorder3 = yBorder * 3; yBorder4 = yBorder * 4;

    xInputOffset = Display_ScaleX(5);
    yInputOffset = Display_ScaleY(2);
    inputExpand  = Display_ScaleX(20);

    caretOffset  = Display_ScaleY(5);
    caretWidth   = Display_ScaleX(10);
    caretHeight  = Display_ScaleY(2);

    scrollbarWidth = Display_ScaleX(10);
    dragPad        = Display_ScaleX(8);
    gridlineWidth  = Display_ScaleX(2);
    gridlineHeight = Display_ScaleY(2);

    hdrYOffset   = Display_ScaleY(3);
    hdrYPadding  = Display_ScaleY(5);
    rowYOffset   = Display_ScaleY(3);
    rowYPadding  = Display_ScaleY(1);

    cellXOffset  = Display_ScaleX(6);
    cellXPadding = Display_ScaleX(5);
    cellMinWidth = Display_ScaleX(20);
    flagXOffset  = Display_ScaleX(2);
    flagYOffset  = Display_ScaleY(6);

    Drawer2D_MakeFont(&titleFont, 16, FONT_FLAGS_BOLD);
    Drawer2D_MakeFont(&textFont,  14, FONT_FLAGS_NONE);
    Drawer2D_MakeFont(&hintFont,  12, FONT_FLAGS_NONE);
    HookEvents();
    LBackend_InitHooks();
}

void LBackend_Free(void) {
    Font_Free(&titleFont);
    Font_Free(&textFont);
    Font_Free(&hintFont);
    Font_Free(&logoFont);
    Font_Free(&rowFont);
}

void LBackend_UpdateLogoFont(void) {
    Font_Free(&logoFont);
    Launcher_MakeLogoFont(&logoFont);
}
void LBackend_DrawLogo(struct Bitmap* bmp, const char* title) {
    Launcher_DrawLogo(&logoFont, title, bmp);
}

void LBackend_SetScreen(struct LScreen* s)   { }
void LBackend_CloseScreen(struct LScreen* s) { }

static void LBackend_LayoutDimensions(struct LWidget* w) {
    const struct LLayout* l = w->layouts + 2;
    while (l->type)
    {
        switch (l->type)
        {
            case LLAYOUT_WIDTH:
                w->width  = WindowInfo.Width  - w->x - Display_ScaleX(l->offset);
                w->width  = max(1, w->width);
                break;
            case LLAYOUT_HEIGHT:
                w->height = WindowInfo.Height - w->y - Display_ScaleY(l->offset);
                w->height = max(1, w->height);
                break;
        }
        l++;
    }
}

static void LBackend_GetLayoutArgs(struct LWidget* w, jvalue* args) {
    const struct LLayout* l = w->layouts;

    args[0].i = l[0].type & 0xFF;
    args[1].i = l[0].offset;
    args[2].i = l[1].type & 0xFF;
    args[3].i = l[1].offset;
}

void LBackend_LayoutWidget(struct LWidget* w) {
    const struct LLayout* l = w->layouts;

    w->x = Gui_CalcPos(l[0].type & 0xFF, Display_ScaleX(l[0].offset), w->width,  WindowInfo.Width);
    w->y = Gui_CalcPos(l[1].type & 0xFF, Display_ScaleY(l[1].offset), w->height, WindowInfo.Height);

    /* e.g. Table widget needs adjusts width/height based on window */
    if (l[1].type & LLAYOUT_EXTRA)
        LBackend_LayoutDimensions(w);

    if (w->type != LWIDGET_TABLE) return;
    LBackend_TableReposition((struct LTable*)w);
}

void LBackend_MarkDirty(void* widget) { }
void LBackend_InitFramebuffer(void) { }
void LBackend_FreeFramebuffer(void) { }

static void JNICALL java_drawBackground(JNIEnv* env, jobject o, jobject bmp) {
    Platform_LogConst("---$$$--");
    AndroidBitmapInfo info;
    void* addr = NULL;

    AndroidBitmap_getInfo(env, bmp, &info);
    AndroidBitmap_lockPixels(env, bmp, &addr);

    struct Bitmap pixels;
    pixels.scan0 = addr;
    pixels.width = info.width;
    pixels.height = info.height;
    Launcher_Active->DrawBackground(Launcher_Active, &pixels);

    AndroidBitmap_unlockPixels(env, bmp);
}

static const JNINativeMethod methods[] = {
        { "drawBackground", "(Landroid/graphics/Bitmap;)V", java_drawBackground }
};

static void LBackend_InitHooks(void) {
    JNIEnv* env;
    JavaGetCurrentEnv(env);
    JavaRegisterNatives(env, methods);
}

void LBackend_Redraw(void) {
    JNIEnv* env;
    JavaGetCurrentEnv(env);
    JavaCallVoid(env, "redrawBackground", "()V", NULL);
}
void LBackend_ThemeChanged(void) { LBackend_Redraw(); }

void LBackend_Tick(void) {
}


/*########################################################################################################################*
*-----------------------------------------------------Event handling------------------------------------------------------*
*#########################################################################################################################*/
static void RequestRedraw(void* obj) { LBackend_Redraw(); }

static void OnPointerDown(void* obj, int idx) {
    Launcher_Active->MouseDown(Launcher_Active, idx);
}

static void OnPointerUp(void* obj, int idx) {
    Launcher_Active->MouseUp(Launcher_Active, idx);
}

static void OnPointerMove(void* obj, int idx) {
    if (!Launcher_Active) return;
    Launcher_Active->MouseMove(Launcher_Active, idx);
}

static void HookEvents(void) {
    Event_Register_(&WindowEvents.Redraw, NULL, RequestRedraw);
    Event_Register_(&PointerEvents.Down,  NULL, OnPointerDown);
    Event_Register_(&PointerEvents.Up,    NULL, OnPointerUp);
    Event_Register_(&PointerEvents.Moved, NULL, OnPointerMove);
}


/*########################################################################################################################*
*------------------------------------------------------ButtonWidget-------------------------------------------------------*
*#########################################################################################################################*/
void LBackend_ButtonInit(struct LButton* w, int width, int height) {
    JNIEnv* env;
    JavaGetCurrentEnv(env);
    jvalue args[4];

    LBackend_GetLayoutArgs(w, args);
    jmethodID method = JavaGetIMethod(env, "buttonAdd", "(IIII)I");
    w->meta = (void*)JavaICall_Int(env, method, args);
    //JavaCallVoid(env, "buttonAdd", "(IIII)V", args);
}

void LBackend_ButtonUpdate(struct LButton* w) {
}

void LBackend_ButtonDraw(struct LButton* w) { }


/*########################################################################################################################*
*-----------------------------------------------------CheckboxWidget------------------------------------------------------*
*#########################################################################################################################*/
void LBackend_CheckboxInit(struct LCheckbox* w) {

}

void LBackend_CheckboxDraw(struct LCheckbox* w) { }


/*########################################################################################################################*
*------------------------------------------------------InputWidget--------------------------------------------------------*
*#########################################################################################################################*/
void LBackend_InputInit(struct LInput* w, int width) {
    JNIEnv* env;
    JavaGetCurrentEnv(env);
    jvalue args[4];

    LBackend_GetLayoutArgs(w, args);
    jmethodID method = JavaGetIMethod(env, "inputAdd", "(IIII)I");
    w->meta = (void*)JavaICall_Int(env, method, args);
    //JavaCallVoid(env, "inputAdd", "(IIII)V", args);
}

void LBackend_InputUpdate(struct LInput* w) {

}

void LBackend_InputTick(struct LInput* w) {
}

void LBackend_InputSelect(struct LInput* w, int idx, cc_bool wasSelected) {
}

void LBackend_InputUnselect(struct LInput* w) {
}

void LBackend_InputDraw(struct LInput* w) { }


/*########################################################################################################################*
*------------------------------------------------------LabelWidget--------------------------------------------------------*
*#########################################################################################################################*/
void LBackend_LabelInit(struct LLabel* w) {

}

void LBackend_LabelUpdate(struct LLabel* w) {

}

void LBackend_LabelDraw(struct LLabel* w) { }


/*########################################################################################################################*
*-------------------------------------------------------LineWidget--------------------------------------------------------*
*#########################################################################################################################*/
void LBackend_LineInit(struct LLine* w, int width) {

}

void LBackend_LineDraw(struct LLine* w) { }


/*########################################################################################################################*
*------------------------------------------------------SliderWidget-------------------------------------------------------*
*#########################################################################################################################*/
void LBackend_SliderInit(struct LSlider* w, int width, int height) {

}

void LBackend_SliderUpdate(struct LSlider* w) {

}

void LBackend_SliderDraw(struct LSlider* w) {}


/*########################################################################################################################*
*-------------------------------------------------------TableWidget-------------------------------------------------------*
*#########################################################################################################################*/
void LBackend_TableInit(struct LTable* w) {

}

void LBackend_TableUpdate(struct LTable* w) {

}

void LBackend_TableReposition(struct LTable* w) {

}

void LBackend_TableFlagAdded(struct LTable* w) {
}

void LBackend_TableDraw(struct LTable* w) { }
void LBackend_TableMouseDown(struct LTable* w, int idx) { }
void LBackend_TableMouseMove(struct LTable* w, int idx) { }
void LBackend_TableMouseUp(struct LTable* w, int idx) { }
#endif
