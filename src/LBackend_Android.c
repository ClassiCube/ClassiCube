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

struct FontDesc logoFont;

static void HookEvents(void);
static void LBackend_InitHooks(void);
void LBackend_Init(void) {
    HookEvents();
    LBackend_InitHooks();
}

void LBackend_Free(void) {
    Font_Free(&logoFont);
}

void LBackend_UpdateLogoFont(void) {
    Font_Free(&logoFont);
    Launcher_MakeLogoFont(&logoFont);
}
void LBackend_DrawLogo(struct Bitmap* bmp, const char* title) {
    Launcher_DrawLogo(&logoFont, title, bmp);
}

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

static void LBackend_GetLayoutArgs(void* widget, jvalue* args) {
    struct LWidget* w = (struct LWidget*)widget;
    const struct LLayout* l = w->layouts;

    args[0].i = l[0].type & 0xFF;
    args[1].i = Display_ScaleX(l[0].offset);
    args[2].i = l[1].type & 0xFF;
    args[3].i = Display_ScaleY(l[1].offset);
}

void LBackend_LayoutWidget(struct LWidget* w) {
    const struct LLayout* l = w->layouts;
    // TODO remove this? once Table is done

    /* e.g. Table widget needs adjusts width/height based on window */
    if (l[1].type & LLAYOUT_EXTRA)
        LBackend_LayoutDimensions(w);
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

static void OnPointerUp(void* obj, int idx) {
    Launcher_Active->MouseUp(Launcher_Active, idx);
}

static void HookEvents(void) {
    Event_Register_(&WindowEvents.Redraw, NULL, RequestRedraw);
    Event_Register_(&PointerEvents.Up,    NULL, OnPointerUp);
}


/*########################################################################################################################*
*------------------------------------------------------ButtonWidget-------------------------------------------------------*
*#########################################################################################################################*/
void LBackend_ButtonInit(struct LButton* w, int width, int height) {
    w->_textWidth  = Display_ScaleX(width);
    w->_textHeight = Display_ScaleY(height);
}

static void LBackend_ButtonShow(struct LButton* w) {
    JNIEnv* env; JavaGetCurrentEnv(env);
    jvalue args[6];

    LBackend_GetLayoutArgs(w, args);
    args[4].i = w->_textWidth;
    args[5].i = w->_textHeight;

    jmethodID method = JavaGetIMethod(env, "buttonAdd", "(IIIIII)I");
    w->meta = (void*)JavaICall_Int(env, method, args);
}

void LBackend_ButtonUpdate(struct LButton* w) {
    JNIEnv* env; JavaGetCurrentEnv(env);
    jvalue args[2];
    if (!w->meta) return;

    args[0].i = (int)w->meta;
    args[1].l = JavaMakeString(env, &w->text);

    // TODO share logic with LabelUpdate/ButtonUpdate
    jmethodID method = JavaGetIMethod(env, "buttonUpdate", "(ILjava/lang/String;)V");
    JavaICall_Void(env, method, args);
    (*env)->DeleteLocalRef(env, args[1].l);
}
void LBackend_ButtonDraw(struct LButton* w) { }


/*########################################################################################################################*
*-----------------------------------------------------CheckboxWidget------------------------------------------------------*
*#########################################################################################################################*/
void LBackend_CheckboxInit(struct LCheckbox* w) { }

static void LBackend_CheckboxShow(struct LCheckbox* w) {
    JNIEnv* env; JavaGetCurrentEnv(env);
    jvalue args[6];

    LBackend_GetLayoutArgs(w, args);
    args[4].l = JavaMakeString(env, &w->text);
    args[5].z = w->value;

    jmethodID method = JavaGetIMethod(env, "checkboxAdd", "(IIIILjava/lang/String;Z)I");
    w->meta = (void*)JavaICall_Int(env, method, args);
    (*env)->DeleteLocalRef(env, args[4].l);
}
void LBackend_CheckboxDraw(struct LCheckbox* w) { }

void LBackend_CheckboxUpdate(struct LCheckbox* w) {
    JNIEnv* env; JavaGetCurrentEnv(env);
    jvalue args[2];
    if (!w->meta) return;

    args[0].i = (int)w->meta;
    args[1].z = w->value;

    // TODO share logic with LabelUpdate/ButtonUpdate
    jmethodID method = JavaGetIMethod(env, "checkboxUpdate", "(IZ)V");
    JavaICall_Void(env, method, args);
}


/*########################################################################################################################*
*------------------------------------------------------InputWidget--------------------------------------------------------*
*#########################################################################################################################*/
void LBackend_InputInit(struct LInput* w, int width) {
    w->_textHeight = Display_ScaleX(width);
}

static void LBackend_InputShow(struct LInput* w) {
    JNIEnv* env; JavaGetCurrentEnv(env);
    jvalue args[6];

    LBackend_GetLayoutArgs(w, args);
    args[4].i = w->_textHeight;
    args[5].i = Display_ScaleY(LINPUT_HEIGHT);

    jmethodID method = JavaGetIMethod(env, "inputAdd", "(IIIIII)I");
    w->meta = (void*)JavaICall_Int(env, method, args);
}

void LBackend_InputUpdate(struct LInput* w) {
    JNIEnv* env; JavaGetCurrentEnv(env);
    jvalue args[2];
    if (!w->meta) return;

    args[0].i = (int)w->meta;
    args[1].l = JavaMakeString(env, &w->text);

    // TODO share logic with LabelUpdate/ButtonUpdate
    jmethodID method = JavaGetIMethod(env, "inputUpdate", "(ILjava/lang/String;)V");
    JavaICall_Void(env, method, args);
    (*env)->DeleteLocalRef(env, args[1].l);
}

void LBackend_InputTick(struct LInput* w) { }
void LBackend_InputSelect(struct LInput* w, int idx, cc_bool wasSelected) { }
void LBackend_InputUnselect(struct LInput* w) { }
void LBackend_InputDraw(struct LInput* w) { }


/*########################################################################################################################*
*------------------------------------------------------LabelWidget--------------------------------------------------------*
*#########################################################################################################################*/
void LBackend_LabelInit(struct LLabel* w) { }

static void LBackend_LabelShow(struct LLabel* w) {
    JNIEnv* env; JavaGetCurrentEnv(env);
    jvalue args[4];
    LBackend_GetLayoutArgs(w, args);

    jmethodID method = JavaGetIMethod(env, "labelAdd", "(IIII)I");
    w->meta = (void*)JavaICall_Int(env, method, args);
}

void LBackend_LabelUpdate(struct LLabel* w) {
    JNIEnv* env; JavaGetCurrentEnv(env);
    jvalue args[2];
    if (!w->meta) return;

    args[0].i = (int)w->meta;
    args[1].l = JavaMakeString(env, &w->text);

    // TODO share logic with LabelUpdate/ButtonUpdate
    jmethodID method = JavaGetIMethod(env, "labelUpdate", "(ILjava/lang/String;)V");
    JavaICall_Void(env, method, args);
    (*env)->DeleteLocalRef(env, args[1].l);
}
void LBackend_LabelDraw(struct LLabel* w) { }


/*########################################################################################################################*
*-------------------------------------------------------LineWidget--------------------------------------------------------*
*#########################################################################################################################*/
void LBackend_LineInit(struct LLine* w, int width) {
    w->_width = Display_ScaleX(width);
}

static void LBackend_LineShow(struct LLine* w) {
    JNIEnv* env; JavaGetCurrentEnv(env);
    jvalue args[7];

    LBackend_GetLayoutArgs(w, args);
    args[4].i = w->_width;
    args[5].i = Display_ScaleY(LLINE_HEIGHT);
    args[6].i = LLine_GetColor();

    jmethodID method = JavaGetIMethod(env, "lineAdd", "(IIIIIII)I");
    w->meta = (void*)JavaICall_Int(env, method, args);
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


/*########################################################################################################################*
*--------------------------------------------------------UIBackend--------------------------------------------------------*
*#########################################################################################################################*/
#define UI_EVENT_CLICKED 1

static struct LWidget* FindWidgetForView(int id) {
    struct LScreen* s = Launcher_Active;
    for (int i = 0; i < s->numWidgets; i++)
    {
        void* meta = s->widgets[i]->meta;
        if (meta != id) continue;

        return s->widgets[i];
    }
    return NULL;
}

extern void LBackend_UIEvent(int id, int cmd) {
    struct LWidget* w = FindWidgetForView(id);
    if (!w) return;

    switch (cmd) {
        case UI_EVENT_CLICKED:
            if (w->OnClick) w->OnClick(w);
            break;
    }
}

static void ShowWidget(struct LWidget* w) {
    switch (w->type) {
        case LWIDGET_BUTTON:
            LBackend_ButtonShow((struct LButton*)w);
            LBackend_ButtonUpdate((struct LButton*)w);
            break;
        case LWIDGET_CHECKBOX:
            LBackend_CheckboxShow((struct LCheckbox*)w);
            break;
        case LWIDGET_INPUT:
            LBackend_InputShow((struct LInput*)w);
            LBackend_InputUpdate((struct LInput*)w);
            break;
        case LWIDGET_LABEL:
            LBackend_LabelShow((struct LLabel*)w);
            LBackend_LabelUpdate((struct LLabel*)w);
            break;
        case LWIDGET_LINE:
            LBackend_LineShow((struct LLine*)w);
            break;
    }
}

void LBackend_SetScreen(struct LScreen* s) {
    for (int i = 0; i < s->numWidgets; i++)
    {
        ShowWidget(s->widgets[i]);
    }
}

void LBackend_CloseScreen(struct LScreen* s) {
    // stop referencing widgets
    for (int i = 0; i < s->numWidgets; i++)
    {
        s->widgets[i]->meta = NULL;
    }

    JNIEnv* env; JavaGetCurrentEnv(env);
    jmethodID method = JavaGetIMethod(env, "clearWidgetsAsync", "()V");
    JavaICall_Void(env, method, NULL);
}
#endif
