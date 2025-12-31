#include "LBackend.h"
#if defined CC_BUILD_ANDROID11111111111
#include "Launcher.h"
#include "Drawer2D.h"
#include "Window.h"
#include "LWidgets.h"
#include "String_.h"
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
#include "android/interop_android.h"

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
void LBackend_DrawLogo(struct Context2D* ctx, const char* title) {
    Launcher_DrawLogo(&logoFont, title, ctx);
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

void LBackend_NeedsRedraw(void* widget) { }
void LBackend_InitFramebuffer(void) { }
void LBackend_FreeFramebuffer(void) { }

static void JNICALL java_drawBackground(JNIEnv* env, jobject o, jobject bmp) {
    Platform_LogConst("---$$$--");
    AndroidBitmapInfo info;
    void* addr = NULL;

    AndroidBitmap_getInfo(env, bmp, &info);
    AndroidBitmap_lockPixels(env, bmp, &addr);

    // TODO refactor this
    struct Context2D ctx;
    struct Bitmap pixels;
    pixels.scan0  = addr;
    pixels.width  = info.width;
    pixels.height = info.height;
    Context2D_Wrap(&ctx, &pixels);

    struct LScreen* s = Launcher_Active;
    if (s) s->DrawBackground(s, &ctx);

    AndroidBitmap_unlockPixels(env, bmp);
}

void LBackend_Redraw(void) {
    JNIEnv* env;
    Java_GetCurrentEnv(env);
	
    jmethodID method = Java_GetIMethod(env, "redrawBackground", "()V");
    Java_ICall_Void(env, method, NULL);
}

static void LBackend_ButtonUpdateBackground(struct LButton* btn);
void LBackend_ThemeChanged(void) {
    struct LScreen* s = Launcher_Active;
    LBackend_Redraw();

    for (int i = 0; i < s->numWidgets; i++)
    {
        struct LWidget* w = s->widgets[i];
        if (w->type != LWIDGET_BUTTON) continue;
        LBackend_ButtonUpdateBackground((struct LButton*)w);
    }
}

void LBackend_Tick(void) { }
void LBackend_AddDirtyFrames(int frames) { }

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

static int ToAndroidColor(BitmapCol color) {
    int R = BitmapCol_R(color);
    int G = BitmapCol_G(color);
    int B = BitmapCol_B(color);
    int A = BitmapCol_A(color);
    return (A << 24) | (R << 16) | (G << 8) | B;
}


static jstring JNICALL java_nextTextPart(JNIEnv* env, jobject o, jstring total, jintArray state) {
    char buffer[NATIVE_STR_LEN];
	// TODO should it really be raw?
    cc_string text = Java_DecodeRaw(env, total, buffer);

    jint* state_ = (*env)->GetIntArrayElements(env, state, NULL);
    text.buffer += state_[0];
    text.length -= state_[0];

    cc_string left = text, part;
    char colorCode = 'f';

    Drawer2D_UNSAFE_NextPart(&text, &part, &colorCode);
    BitmapCol color = Drawer2D_GetColor(colorCode);

    state_[0] += left.length - text.length;
    state_[1] = ToAndroidColor(color);

    (*env)->ReleaseIntArrayElements(env, state, state_, 0);
    return Java_AllocString(env, &part);
}


/*########################################################################################################################*
*-----------------------------------------------------Event handling------------------------------------------------------*
*#########################################################################################################################*/
static void RequestRedraw(void* obj) { LBackend_Redraw(); }

static void OnPointerUp(void* obj, int idx) {
    Launcher_Active->MouseUp(Launcher_Active, idx);
}

static void OnWindowCreated(void* obj) {
    // e.g. after pause and resume
    // TODO should pause/resume not trigger launcher screen recreation?
    if (Launcher_Active) Launcher_SetScreen(Launcher_Active);
}

static void HookEvents(void) {
    Event_Register_(&WindowEvents.RedrawNeeded, NULL, RequestRedraw);
    Event_Register_(&PointerEvents.Up,          NULL, OnPointerUp);
    Event_Register_(&WindowEvents.Created,      NULL, OnWindowCreated);
}


/*########################################################################################################################*
*------------------------------------------------------ButtonWidget-------------------------------------------------------*
*#########################################################################################################################*/
void LBackend_ButtonInit(struct LButton* w, int width, int height) {
    w->_textWidth  = Display_ScaleX(width);
    w->_textHeight = Display_ScaleY(height);
}

static void LBackend_ButtonShow(struct LButton* w) {
    JNIEnv* env; Java_GetCurrentEnv(env);
    jvalue args[6];

    LBackend_GetLayoutArgs(w, args);
    args[4].i = w->_textWidth;
    args[5].i = w->_textHeight;

    jmethodID method = Java_GetIMethod(env, "buttonAdd", "(IIIIII)I");
    w->meta = (void*)Java_ICall_Int(env, method, args);
}

void LBackend_ButtonUpdate(struct LButton* w) {
    JNIEnv* env; Java_GetCurrentEnv(env);
    jvalue args[2];
    if (!w->meta) return;

    args[0].i = (int)w->meta;
    args[1].l = Java_AllocString(env, &w->text);

    // TODO share logic with LabelUpdate/ButtonUpdate
    jmethodID method = Java_GetIMethod(env, "buttonUpdate", "(ILjava/lang/String;)V");
    Java_ICall_Void(env, method, args);
	
    Java_DeleteLocalRef(env, args[1].l);
}
void LBackend_ButtonDraw(struct LButton* w) { }

static void JNICALL java_makeButtonActive(JNIEnv* env, jobject o, jobject bmp) {
    Platform_LogConst("---&&&--");
    AndroidBitmapInfo info;
    void* addr = NULL;

    // TODO share code with drawBackground
    AndroidBitmap_getInfo(env, bmp, &info);
    AndroidBitmap_lockPixels(env, bmp, &addr);

    // TODO refactor this
    struct Context2D ctx;
    struct Bitmap pixels;
    pixels.scan0  = addr;
    pixels.width  = info.width;
    pixels.height = info.height;
    Context2D_Wrap(&ctx, &pixels);

    LButton_DrawBackground(&ctx, 0, 0, info.width, info.height, true);
    AndroidBitmap_unlockPixels(env, bmp);
}

static void JNICALL java_makeButtonDefault(JNIEnv* env, jobject o, jobject bmp) {
    Platform_LogConst("---####--");
    AndroidBitmapInfo info;
    void* addr = NULL;

    // TODO share code with drawBackground
    AndroidBitmap_getInfo(env, bmp, &info);
    AndroidBitmap_lockPixels(env, bmp, &addr);

    // TODO refactor this
    struct Context2D ctx;
    struct Bitmap pixels;
    pixels.scan0  = addr;
    pixels.width  = info.width;
    pixels.height = info.height;
    Context2D_Wrap(&ctx, &pixels);

    LButton_DrawBackground(&ctx, 0, 0, info.width, info.height, false);
    AndroidBitmap_unlockPixels(env, bmp);
}

static void LBackend_ButtonUpdateBackground(struct LButton* w) {
    JNIEnv* env; Java_GetCurrentEnv(env);
    jvalue args[1];
    if (!w->meta) return;

    args[0].i = (int)w->meta;
    jmethodID method = Java_GetIMethod(env, "buttonUpdateBackground", "(I)V");
    Java_ICall_Void(env, method, args);
}


/*########################################################################################################################*
*-----------------------------------------------------CheckboxWidget------------------------------------------------------*
*#########################################################################################################################*/
void LBackend_CheckboxInit(struct LCheckbox* w) { }

static void LBackend_CheckboxShow(struct LCheckbox* w) {
    JNIEnv* env; Java_GetCurrentEnv(env);
    jvalue args[6];

    LBackend_GetLayoutArgs(w, args);
    args[4].l = Java_AllocString(env, &w->text);
    args[5].z = w->value;

    jmethodID method = Java_GetIMethod(env, "checkboxAdd", "(IIIILjava/lang/String;Z)I");
    w->meta = (void*)Java_ICall_Int(env, method, args);
    Java_DeleteLocalRef(env, args[4].l);
}
void LBackend_CheckboxDraw(struct LCheckbox* w) { }

void LBackend_CheckboxUpdate(struct LCheckbox* w) {
    JNIEnv* env; Java_GetCurrentEnv(env);
    jvalue args[2];
    if (!w->meta) return;

    args[0].i = (int)w->meta;
    args[1].z = w->value;

    // TODO share logic with LabelUpdate/ButtonUpdate
    jmethodID method = Java_GetIMethod(env, "checkboxUpdate", "(IZ)V");
    Java_ICall_Void(env, method, args);
}


/*########################################################################################################################*
*------------------------------------------------------InputWidget--------------------------------------------------------*
*#########################################################################################################################*/
void LBackend_InputInit(struct LInput* w, int width) {
    w->_textHeight = Display_ScaleX(width);
}

static void LBackend_InputShow(struct LInput* w) {
    JNIEnv* env; Java_GetCurrentEnv(env);
    jvalue args[8];

    LBackend_GetLayoutArgs(w, args);
    args[4].i = w->_textHeight;
    args[5].i = Display_ScaleY(LINPUT_HEIGHT);
    args[6].i = w->inputType;
    args[7].l = Java_AllocConst(env, w->hintText);

    jmethodID method = Java_GetIMethod(env, "inputAdd", "(IIIIIIILjava/lang/String;)I");
    w->meta = (void*)Java_ICall_Int(env, method, args);
    Java_DeleteLocalRef(env, args[7].l);
}

void LBackend_InputUpdate(struct LInput* w) {
    JNIEnv* env; Java_GetCurrentEnv(env);
    jvalue args[2];
    if (!w->meta) return;

    args[0].i = (int)w->meta;
    args[1].l = Java_AllocString(env, &w->text);

    // TODO share logic with LabelUpdate/ButtonUpdate
    jmethodID method = Java_GetIMethod(env, "inputUpdate", "(ILjava/lang/String;)V");
    Java_ICall_Void(env, method, args);
    Java_DeleteLocalRef(env, args[1].l);
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
    JNIEnv* env; Java_GetCurrentEnv(env);
    jvalue args[4];
    LBackend_GetLayoutArgs(w, args);

    jmethodID method = Java_GetIMethod(env, "labelAdd", "(IIII)I");
    w->meta = (void*)Java_ICall_Int(env, method, args);
}

void LBackend_LabelUpdate(struct LLabel* w) {
    JNIEnv* env; Java_GetCurrentEnv(env);
    jvalue args[2];
    if (!w->meta) return;

    args[0].i = (int)w->meta;
    args[1].l = Java_AllocString(env, &w->text);

    // TODO share logic with LabelUpdate/ButtonUpdate
    jmethodID method = Java_GetIMethod(env, "labelUpdate", "(ILjava/lang/String;)V");
    Java_ICall_Void(env, method, args);
    Java_DeleteLocalRef(env, args[1].l);
}
void LBackend_LabelDraw(struct LLabel* w) { }


/*########################################################################################################################*
*-------------------------------------------------------LineWidget--------------------------------------------------------*
*#########################################################################################################################*/
void LBackend_LineInit(struct LLine* w, int width) {
    w->_width = Display_ScaleX(width);
}

static void LBackend_LineShow(struct LLine* w) {
    JNIEnv* env; Java_GetCurrentEnv(env);
    jvalue args[7];

    LBackend_GetLayoutArgs(w, args);
    args[4].i = w->_width;
    args[5].i = Display_ScaleY(LLINE_HEIGHT);
    args[6].i = ToAndroidColor(LLine_GetColor());

    jmethodID method = Java_GetIMethod(env, "lineAdd", "(IIIIIII)I");
    w->meta = (void*)Java_ICall_Int(env, method, args);
}
void LBackend_LineDraw(struct LLine* w) { }


/*########################################################################################################################*
*------------------------------------------------------SliderWidget-------------------------------------------------------*
*#########################################################################################################################*/
void LBackend_SliderInit(struct LSlider* w, int width, int height) {
    w->_width  = Display_ScaleX(width);
    w->_height = Display_ScaleY(height);
}

static void LBackend_SliderShow(struct LSlider* w) {
    JNIEnv* env; Java_GetCurrentEnv(env);
    jvalue args[7];

    LBackend_GetLayoutArgs(w, args);
    args[4].i = w->_width;
    args[5].i = w->_height;
    args[6].i = ToAndroidColor(w->color);

    jmethodID method = Java_GetIMethod(env, "sliderAdd", "(IIIIIII)I");
    w->meta = (void*)Java_ICall_Int(env, method, args);
}

void LBackend_SliderUpdate(struct LSlider* w) {
    JNIEnv* env; Java_GetCurrentEnv(env);
    jvalue args[2];
    if (!w->meta) return;

    args[0].i = (int)w->meta;
    args[1].i = w->value;

    jmethodID method = Java_GetIMethod(env, "sliderUpdate", "(II)V");
    Java_ICall_Void(env, method, args);
}
void LBackend_SliderDraw(struct LSlider* w) {}


/*########################################################################################################################*
*-------------------------------------------------------TableWidget-------------------------------------------------------*
*#########################################################################################################################*/
void LBackend_TableInit(struct LTable* w) {

}

static void LBackend_TableShow(struct LTable* w) {
    JNIEnv* env; Java_GetCurrentEnv(env);
    jvalue args[5];

    LBackend_GetLayoutArgs(w, args);
    args[4].i = ToAndroidColor(LTable_RowColor(1, false, false));

    jmethodID method = Java_GetIMethod(env, "tableAdd", "(IIIII)I");
    w->meta = (void*)Java_ICall_Int(env, method, args);
    LBackend_TableUpdate(w);
}

static jstring GetTableDetails(JNIEnv* env, struct ServerInfo* server) {
    char buffer[NATIVE_STR_LEN];
    cc_string text = String_FromArray(buffer);

    String_Format2(&text, "%i/%i players, up for ", &server->players, &server->maxPlayers);
    LTable_FormatUptime(&text, server->uptime);
    if (server->software.length) String_Format1(&text, " | %s", &server->software);

    return Java_AllocString(env, &text);
}

void LBackend_TableUpdate(struct LTable* w) {
    JNIEnv* env; Java_GetCurrentEnv(env);
    jvalue args[3];
    jmethodID method;

    method = Java_GetIMethod(env, "tableStartUpdate", "()V");
    Java_ICall_Void(env, method, args);
    method = Java_GetIMethod(env, "tableAddEntry", "(Ljava/lang/String;Ljava/lang/String;Z)V");

    for (int i = 0; i < w->rowsCount; i++)
    {
        struct ServerInfo* info = LTable_Get(i);
        args[0].l = Java_AllocString(env, &info->name);
        args[1].l = GetTableDetails(env, info);
        args[2].z = info->featured;
        Java_ICall_Void(env, method, args);

        Java_DeleteLocalRef(env, args[0].l);
        Java_DeleteLocalRef(env, args[1].l);
    }

    method    = Java_GetIMethod(env, "tableFinishUpdate", "(I)V");
    args[0].i = (int)w->meta;
    Java_ICall_Void(env, method, args);
}

void LBackend_TableReposition(struct LTable* w) {

}

void LBackend_TableFlagAdded(struct LTable* w) {
}

void LBackend_TableDraw(struct LTable* w) { }
void LBackend_TableMouseDown(struct LTable* w, int idx) { }
void LBackend_TableMouseMove(struct LTable* w, int idx) { }
void LBackend_TableMouseUp(struct LTable* w, int idx) { }

static jint JNICALL java_tableGetColor(JNIEnv* env, jobject o, jint row, jboolean selected, jboolean featured) {
    return ToAndroidColor(LTable_RowColor(row, selected, featured));
}


/*########################################################################################################################*
*--------------------------------------------------------UIBackend--------------------------------------------------------*
*#########################################################################################################################*/
#define UI_EVENT_CLICKED 1
#define UI_EVENT_CHANGED 2

static void JNICALL java_UIClicked(JNIEnv* env, jobject o, jint id) {
    struct LWidget* w = FindWidgetForView(id);
    if (!w) return;

    if (w->OnClick) w->OnClick(w);
}

static void JNICALL java_UIChanged(JNIEnv* env, jobject o, jint id, jint val) {
    struct LCheckbox* cb = (struct LCheckbox*)FindWidgetForView(id);
    if (!cb) return;

    cb->value = val;
    if (cb->ValueChanged) cb->ValueChanged(cb);
}

static void JNICALL java_UIString(JNIEnv* env, jobject o, jint id, jstring str) {
    struct LInput* ipt = (struct LInput*)FindWidgetForView(id);
    if (!ipt) return;

    char buffer[NATIVE_STR_LEN];
    cc_string text = Java_GetString(env, str, buffer);
    String_Copy(&ipt->text, &text);
    if (ipt->TextChanged) ipt->TextChanged(ipt);
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
        case LWIDGET_SLIDER:
            LBackend_SliderShow((struct LSlider*)w);
            break;
        case LWIDGET_TABLE:
            LBackend_TableShow((struct LTable*)w);
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

    JNIEnv* env; Java_GetCurrentEnv(env);
    jmethodID method = Java_GetIMethod(env, "clearWidgetsAsync", "()V");
    Java_ICall_Void(env, method, NULL);
}

static const JNINativeMethod methods[] = {
        { "nextTextPart",       "(Ljava/lang/String;[I)Ljava/lang/String;", java_nextTextPart },
        { "drawBackground",     "(Landroid/graphics/Bitmap;)V", java_drawBackground },
        { "makeButtonActive",   "(Landroid/graphics/Bitmap;)V", java_makeButtonActive },
        { "makeButtonDefault",  "(Landroid/graphics/Bitmap;)V", java_makeButtonDefault },
        { "processOnUIClicked", "(I)V", java_UIClicked },
        { "processOnUIChanged", "(II)V", java_UIChanged },
        { "processOnUIString",  "(ILjava/lang/String;)V", java_UIString },
        { "tableGetColor",      "(IZZ)I", java_tableGetColor },
};

static void LBackend_InitHooks(void) {
    JNIEnv* env;
    Java_GetCurrentEnv(env);
    JavaRegisterNatives(env, methods);
}
#endif
