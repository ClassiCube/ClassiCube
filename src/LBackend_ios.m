#include "Core.h"
#if defined CC_BUILD_IOS
#include "Bitmap.h"
#include "Input.h"
#include "Platform.h"
#include "String.h"
#include "Errors.h"
#include "Drawer2D.h"
#include "Launcher.h"
#include "LBackend.h"
#include "LWidgets.h"
#include "LScreens.h"
#include "Gui.h"
#include "LWeb.h"
#include "Funcs.h"
#include "Window.h"
#include <UIKit/UIKit.h>
#include <CoreText/CoreText.h>

#ifdef TARGET_OS_TV
	// NSFontAttributeName etc - iOS 6.0
	#define TEXT_ATTRIBUTE_FONT  NSFontAttributeName
	#define TEXT_ATTRIBUTE_COLOR NSForegroundColorAttributeName
#else
	// UITextAttributeFont etc - iOS 5.0
	#define TEXT_ATTRIBUTE_FONT  UITextAttributeFont
	#define TEXT_ATTRIBUTE_COLOR UITextAttributeTextColor
#endif

// shared state with interop_ios.m
extern UITextField* kb_widget;
extern CGContextRef win_ctx;
extern UIView* view_handle;

UIColor* ToUIColor(BitmapCol color, float A);
NSString* ToNSString(const cc_string* text);
void LInput_SetKeyboardType(UITextField* fld, int flags);
void LInput_SetPlaceholder(UITextField* fld, const char* placeholder);


/*########################################################################################################################*
 *----------------------------------------------------Common helpers------------------------------------------------------*
 *#########################################################################################################################*/
static NSMutableAttributedString* ToAttributedString(const cc_string* text) {
    // NSMutableAttributedString - iOS 3.2
    cc_string left = *text, part;
    char colorCode = 'f';
    NSMutableAttributedString* str = [[NSMutableAttributedString alloc] init];
    
    while (Drawer2D_UNSAFE_NextPart(&left, &part, &colorCode))
    {
        BitmapCol color = Drawer2D_GetColor(colorCode);
        NSString* bit   = ToNSString(&part);
        NSDictionary* attrs =
        @{
          //TEXT_ATTRIBUTE_FONT : font,
          TEXT_ATTRIBUTE_COLOR  : ToUIColor(color, 1.0f)
        };
        NSAttributedString* attr_bit = [[NSAttributedString alloc] initWithString:bit attributes:attrs];
        [str appendAttributedString:attr_bit];
    }
    return str;
}


static UIColor* GetStringColor(const cc_string* text) {
    cc_string left = *text, part;
    char colorCode = 'f';
    Drawer2D_UNSAFE_NextPart(&left, &part, &colorCode);
    
    BitmapCol color = Drawer2D_GetColor(colorCode);
    return ToUIColor(color, 1.0f);
}

static NSString* GetColorlessString(const cc_string* text) {
    char buffer[128];
    cc_string tmp = String_FromArray(buffer);

    String_AppendColorless(&tmp, text);
    return ToNSString(&tmp);
}


static void FreeContents(void* info, const void* data, size_t size) { Mem_Free(data); }
// TODO probably a better way..
static UIImage* ToUIImage(struct Bitmap* bmp) {
    CGColorSpaceRef colorspace = CGColorSpaceCreateDeviceRGB();
    CGDataProviderRef provider;
    CGImageRef image;

    provider = CGDataProviderCreateWithData(NULL, bmp->scan0,
                                            Bitmap_DataSize(bmp->width, bmp->height), FreeContents);
    image    = CGImageCreate(bmp->width, bmp->height, 8, 32, bmp->width * 4, colorspace,
                             kCGBitmapByteOrder32Host | kCGImageAlphaNoneSkipFirst, provider, NULL, 0, 0);
    
    UIImage* img = [UIImage imageWithCGImage:image];
    
    CGImageRelease(image);
    CGDataProviderRelease(provider);
    CGColorSpaceRelease(colorspace);
    return img;
}


/*########################################################################################################################*
 *------------------------------------------------------UI Backend--------------------------------------------------------*
 *#########################################################################################################################*/
static struct LWidget* FindWidgetForView(id obj) {
    struct LScreen* s = Launcher_Active;
    for (int i = 0; i < s->numWidgets; i++)
    {
        void* meta = s->widgets[i]->meta;
        if (meta != (__bridge void*)obj) continue;
        
        return s->widgets[i];
    }
    return NULL;
}

static void LTable_UpdateCellColor(UIView* view, struct ServerInfo* server, int row, cc_bool selected);
static void LTable_UpdateCell(UITableView* table, UITableViewCell* cell, int row);

static NSString* cellID = @"CC_Cell";
@interface CCUIController : NSObject<UITableViewDataSource, UITableViewDelegate, UITextFieldDelegate>
@end

@implementation CCUIController

- (void)handleButtonPress:(id)sender {
    struct LWidget* w = FindWidgetForView(sender);
    if (!w) return;
        
    struct LButton* btn = (struct LButton*)w;
    btn->OnClick(btn);
}

- (void)handleTextChanged:(id)sender {
    struct LWidget* w = FindWidgetForView(sender);
    if (!w) return;
    
    UITextField* src   = (UITextField*)sender;
    const char* str    = src.text.UTF8String;
    struct LInput* ipt = (struct LInput*)w;
    
    ipt->text.length = 0;
    String_AppendUtf8(&ipt->text, str, String_Length(str));
    if (ipt->TextChanged) ipt->TextChanged(ipt);
}

- (void)handleValueChanged:(id)sender {
    UISwitch* swt     = (UISwitch*)sender;
    UIView* parent    = swt.superview;
    struct LWidget* w = FindWidgetForView(parent);
    if (!w) return;

    struct LCheckbox* cb = (struct LCheckbox*)w;
    cb->value = [swt isOn];
    if (!cb->ValueChanged) return;
    cb->ValueChanged(cb);
}

// === UITableViewDataSource ===
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    // cellForRowAtIndexPath - iOS 2.0
    //UITableViewCell* cell = [tableView dequeueReusableCellWithIdentifier:cellID forIndexPath:indexPath];
    UITableViewCell* cell = [tableView dequeueReusableCellWithIdentifier:cellID];
    if (cell == nil) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:cellID];
    }
    
    LTable_UpdateCell(tableView, cell, (int)indexPath.row);
    return cell;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    // numberOfRowsInSection - iOS 2.0
    struct LTable* w = (struct LTable*)FindWidgetForView(tableView);
    return w ? w->rowsCount : 0;
}

// === UITableViewDelegate ===
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    // didSelectRowAtIndexPath - iOS 2.0
    int row = (int)indexPath.row;
    struct ServerInfo* server = LTable_Get(row);
    LTable_UpdateCellColor([tableView cellForRowAtIndexPath:indexPath], server, row, true);
    
    struct LTable* w = (struct LTable*)FindWidgetForView(tableView);
    if (!w) return;
    LTable_RowClick(w, row);
}

- (void)tableView:(UITableView *)tableView didDeselectRowAtIndexPath:(NSIndexPath *)indexPath {
    // didDeselectRowAtIndexPath - iOS 2.0
    int row = (int)indexPath.row;
    struct ServerInfo* server = LTable_Get(row);
    LTable_UpdateCellColor([tableView cellForRowAtIndexPath:indexPath], server, row, false);
}

// === UITextFieldDelegate ===
- (BOOL)textFieldShouldReturn:(UITextField *)textField {
    // textFieldShouldReturn - iOS 2.0
    struct LWidget* w   = FindWidgetForView(textField);
    if (!w) return YES;
    struct LWidget* sel = Launcher_Active->onEnterWidget;
    
    if (sel && !w->skipsEnter) {
        sel->OnClick(sel);
    } else {
        [textField resignFirstResponder];
    }
    return YES;
}

- (void)textFieldDidBeginEditing:(UITextField *)textField {
    // textFieldDidBeginEditing - iOS 2.0
    kb_widget = textField;
}

@end

static CCUIController* ui_controller;
void LBackend_Init(void) {
    ui_controller = [[CCUIController alloc] init];
    CFBridgingRetain(ui_controller); // prevent GC TODO even needed?
}

void LBackend_NeedsRedraw(void* widget) { }
void LBackend_Tick(void) { }
void LBackend_Free(void) { }
void LBackend_UpdateTitleFont(void) { }
void LBackend_AddDirtyFrames(int frames) { }

static void DrawText(NSAttributedString* str, struct Context2D* ctx, int x, int y) {
    // CTLineCreateWithAttributedString - iOS 3.2
    CTLineRef line = CTLineCreateWithAttributedString((CFAttributedStringRef)str);
    CGRect bounds  = CTLineGetImageBounds(line, win_ctx);
    int centreX    = (int)(ctx->width / 2.0f - bounds.size.width / 2.0f);
    
    CGContextSetTextPosition(win_ctx, centreX + x, ctx->height - y);
    CTLineDraw(line, win_ctx);
}

void LBackend_DrawTitle(struct Context2D* ctx, const char* title) {
    if (Launcher_BitmappedText()) {
        struct FontDesc font;
        Launcher_MakeTitleFont(&font);
        Launcher_DrawTitle(&font, title, ctx);
        // bitmapped fonts don't need to be freed
        return;
    }
    
    // systemFontOfSize: - iOS 2.0
    UIFont* font   = [UIFont systemFontOfSize:42];
    NSString* text = [NSString stringWithCString:title encoding:NSASCIIStringEncoding];
        
    NSDictionary* attrs_bg =
    @{
      TEXT_ATTRIBUTE_FONT  : font,
      TEXT_ATTRIBUTE_COLOR : UIColor.blackColor
    };
    NSAttributedString* str_bg = [[NSAttributedString alloc] initWithString:text attributes:attrs_bg];
    DrawText(str_bg, ctx, 4, 42);
        
    NSDictionary* attrs_fg =
    @{
      TEXT_ATTRIBUTE_FONT  : font,
      TEXT_ATTRIBUTE_COLOR : UIColor.whiteColor
    };
    NSAttributedString* str_fg = [[NSAttributedString alloc] initWithString:text attributes:attrs_fg];
    DrawText(str_fg, ctx, 0, 38);
}

void LBackend_InitFramebuffer(void) { }
void LBackend_FreeFramebuffer(void) { }

void LBackend_Redraw(void) {
	struct Context2D ctx;
	struct Bitmap bmp;
	int width  = max(Window_Main.Width,  1);
	int height = max(Window_Main.Height, 1);
	
	Window_AllocFramebuffer(&bmp, width, height);
		Context2D_Wrap(&ctx, &bmp);
		Launcher_Active->DrawBackground(Launcher_Active, &ctx);
		Rect2D rect = { 0, 0, width, height };
		Window_DrawFramebuffer(rect, &bmp);
	Window_FreeFramebuffer(&bmp);
}

static void LBackend_ButtonUpdateBackground(struct LButton* w);
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

/*########################################################################################################################*
 *------------------------------------------------------ButtonWidget-------------------------------------------------------*
 *#########################################################################################################################*/
static void LBackend_ButtonUpdateBackground(struct LButton* w) {
    UIButton* btn = (__bridge UIButton*)w->meta;
    CGRect rect   = [btn frame];
    int width     = (int)rect.size.width;
    int height    = (int)rect.size.height;
    // memory freeing deferred until UIImage is freed (see FreeContents)
    struct Bitmap bmp1, bmp2;
    struct Context2D ctx1, ctx2;
    
    Bitmap_Allocate(&bmp1, width, height);
    Context2D_Wrap(&ctx1, &bmp1);
    LButton_DrawBackground(&ctx1, 0, 0, width, height, false);
    [btn setBackgroundImage:ToUIImage(&bmp1) forState:UIControlStateNormal];
    
    Bitmap_Allocate(&bmp2, width, height);
    Context2D_Wrap(&ctx2, &bmp2);
    LButton_DrawBackground(&ctx2, 0, 0, width, height, true);
    [btn setBackgroundImage:ToUIImage(&bmp2) forState:UIControlStateHighlighted];
}

void LBackend_ButtonInit(struct LButton* w, int width, int height) {
    w->_textWidth  = width;
    w->_textHeight = height;
}

static UIView* LBackend_ButtonShow(struct LButton* w) {
    UIButton* btn = [[UIButton alloc] init];
    btn.frame = CGRectMake(0, 0, w->_textWidth, w->_textHeight);
    [btn addTarget:ui_controller action:@selector(handleButtonPress:) forControlEvents:UIControlEventTouchUpInside];
    
    w->meta = (__bridge void*)btn;
    LBackend_ButtonUpdateBackground(w);
    LBackend_ButtonUpdate(w);
    return btn;
}

void LBackend_ButtonUpdate(struct LButton* w) {
    UIButton* btn = (__bridge UIButton*)w->meta;
    
    UIColor* color = GetStringColor(&w->text);
    [btn setTitleColor:color forState:UIControlStateNormal];
    
    NSString* str = GetColorlessString(&w->text);
    [btn setTitle:str forState:UIControlStateNormal];
}
void LBackend_ButtonDraw(struct LButton* w) { }


/*########################################################################################################################*
 *-----------------------------------------------------CheckboxWidget------------------------------------------------------*
 *#########################################################################################################################*/
void LBackend_CheckboxInit(struct LCheckbox* w) { }

static UIView* LBackend_CheckboxShow(struct LCheckbox* w) {
    UIView* root  = [[UIView alloc] init];
    CGRect frame;
    
    UISwitch* swt = [[UISwitch alloc] init];
    [swt addTarget:ui_controller action:@selector(handleValueChanged:) forControlEvents:UIControlEventValueChanged];
    
    UILabel* lbl  = [[UILabel alloc] init];
    lbl.backgroundColor = UIColor.clearColor;
    lbl.textColor = UIColor.whiteColor;
    lbl.text      = ToNSString(&w->text);
    [lbl sizeToFit]; // adjust label to fit text
    
    [root addSubview:swt];
    [root addSubview:lbl];
    
    // label should be slightly to right of switch and vertically centred
    frame = lbl.frame;
    frame.origin.x = swt.frame.size.width + 10.0f;
    frame.origin.y = swt.frame.size.height / 2 - frame.size.height / 2;
    lbl.frame = frame;
    
    // adjust root view height to enclose children
    frame = root.frame;
    frame.size.width  = lbl.frame.origin.x + lbl.frame.size.width;
    frame.size.height = max(swt.frame.size.height, lbl.frame.size.height);
    root.frame = frame;
    
    //root.userInteractionEnabled = YES;
    w->meta = (__bridge void*)root;
    LBackend_CheckboxUpdate(w);
    return root;
}

void LBackend_CheckboxUpdate(struct LCheckbox* w) {
    UIView* root  = (__bridge UIView*)w->meta;
    UISwitch* swt = (UISwitch*)root.subviews[0];
    
    swt.on = w->value;
}
void LBackend_CheckboxDraw(struct LCheckbox* w) { }


/*########################################################################################################################*
 *------------------------------------------------------InputWidget--------------------------------------------------------*
 *#########################################################################################################################*/
void LInput_SetKeyboardType(UITextField* fld, int flags) {
    int type = flags & 0xFF;
    if (type == KEYBOARD_TYPE_INTEGER) {
        fld.keyboardType = UIKeyboardTypeNumberPad;
    } else if (type == KEYBOARD_TYPE_PASSWORD) {
        fld.secureTextEntry = YES;
    }
    
    if (flags & KEYBOARD_FLAG_SEND) {
        fld.returnKeyType = UIReturnKeySend;
    } else {
        fld.returnKeyType = UIReturnKeyDone;
    }
}

void LInput_SetPlaceholder(UITextField* fld, const char* placeholder) {
    if (!placeholder) return;
    
    cc_string hint  = String_FromReadonly(placeholder);
    fld.placeholder = ToNSString(&hint);
}

void LBackend_InputInit(struct LInput* w, int width) {
    w->_textHeight = width;
}

static UIView* LBackend_InputShow(struct LInput* w) {
    UITextField* fld = [[UITextField alloc] init];
    fld.frame           = CGRectMake(0, 0, w->_textHeight, LINPUT_HEIGHT);
    fld.borderStyle     = UITextBorderStyleBezel;
    fld.backgroundColor = UIColor.whiteColor;
    fld.textColor       = UIColor.blackColor;
    fld.delegate        = ui_controller;
    [fld addTarget:ui_controller action:@selector(handleTextChanged:) forControlEvents:UIControlEventEditingChanged];
    
    LInput_SetKeyboardType(fld, w->inputType);
    LInput_SetPlaceholder(fld,  w->hintText);
    
    w->meta = (__bridge void*)fld;
    LBackend_InputUpdate(w);
    return fld;
}

void LBackend_InputUpdate(struct LInput* w) {
    UITextField* fld = (__bridge UITextField*)w->meta;
    fld.text         = ToNSString(&w->text);
}

void LBackend_InputDraw(struct LInput* w) { }
void LBackend_InputTick(struct LInput* w) { }
void LBackend_InputSelect(struct LInput* w, int idx, cc_bool wasSelected) { }
void LBackend_InputUnselect(struct LInput* w) { }


/*########################################################################################################################*
 *------------------------------------------------------LabelWidget--------------------------------------------------------*
 *#########################################################################################################################*/
void LBackend_LabelInit(struct LLabel* w) { }

static UIView* LBackend_LabelShow(struct LLabel* w) {
    UILabel* lbl  = [[UILabel alloc] init];
    w->meta       = (__bridge void*)lbl;
    lbl.backgroundColor = UIColor.clearColor;
    
    if (w->small) lbl.font = [UIFont systemFontOfSize:14.0f];
    LBackend_LabelUpdate(w);
    return lbl;
}

void LBackend_LabelUpdate(struct LLabel* w) {
    UILabel* lbl = (__bridge UILabel*)w->meta;
    if (!lbl) return;
    
    if ([lbl respondsToSelector:@selector(attributedText)]) {
        // attributedText - iOS 6.0
        lbl.attributedText = ToAttributedString(&w->text);
    } else {
        lbl.textColor = GetStringColor(&w->text);
        lbl.text      = GetColorlessString(&w->text);
    }
    
    [lbl sizeToFit]; // adjust label to fit text
}
void LBackend_LabelDraw(struct LLabel* w) { }


/*########################################################################################################################*
 *-------------------------------------------------------LineWidget--------------------------------------------------------*
 *#########################################################################################################################*/
void LBackend_LineInit(struct LLine* w, int width) {
    w->_width = width;
}

static UIView* LBackend_LineShow(struct LLine* w) {
    UIView* view = [[UIView alloc] init];
    view.frame   = CGRectMake(0, 0, w->_width, LLINE_HEIGHT);
    w->meta      = (__bridge void*)view;
    
    BitmapCol color      = LLine_GetColor();
    view.backgroundColor = ToUIColor(color, 0.5f);
    return view;
}
void LBackend_LineDraw(struct LLine* w) { }


/*########################################################################################################################*
 *------------------------------------------------------SliderWidget-------------------------------------------------------*
 *#########################################################################################################################*/
void LBackend_SliderInit(struct LSlider* w, int width, int height) {
    w->_width  = width;
    w->_height = height;
}

static UIView* LBackend_SliderShow(struct LSlider* w) {
    UIProgressView* prg = [[UIProgressView alloc] init];
    prg.frame           = CGRectMake(0, 0, w->_width, w->_height);
    prg.progressTintColor = ToUIColor(w->color, 1.0f);
    
    w->meta = (__bridge void*)prg;
    return prg;
}

void LBackend_SliderUpdate(struct LSlider* w) {
    UIProgressView* prg = (__bridge UIProgressView*)w->meta;
    
    prg.progress = w->value / 100.0f;
}
void LBackend_SliderDraw(struct LSlider* w) { }


/*########################################################################################################################*
 *------------------------------------------------------TableWidget-------------------------------------------------------*
 *#########################################################################################################################*/
void LBackend_TableInit(struct LTable* w) { }

static UIView* LBackend_TableShow(struct LTable* w) {
    UITableView* tbl = [[UITableView alloc] init];
    tbl.delegate   = ui_controller;
    tbl.dataSource = ui_controller;
    tbl.editing    = NO;
    tbl.allowsSelection = YES;
    LTable_UpdateCellColor(tbl, NULL, 1, false);
    
    //[tbl registerClass:UITableViewCell.class forCellReuseIdentifier:cellID];
    w->meta = (__bridge void*)tbl;
    return tbl;
}

void LBackend_TableUpdate(struct LTable* w) {
    UITableView* tbl = (__bridge UITableView*)w->meta;
    [tbl reloadData];
}

void LBackend_TableDraw(struct LTable* w) { }
void LBackend_TableReposition(struct LTable* w) { }
void LBackend_TableMouseDown(struct LTable* w, int idx) { }
void LBackend_TableMouseUp(struct   LTable* w, int idx) { }
void LBackend_TableMouseMove(struct LTable* w, int idx) { }

static void LTable_UpdateCellColor(UIView* view, struct ServerInfo* server, int row, cc_bool selected) {
    BitmapCol color = LTable_RowColor(row, selected, server && server->featured);
    if (color) {
        view.backgroundColor = ToUIColor(color, 1.0f);
        view.opaque          = YES;
    } else {
        view.backgroundColor = UIColor.clearColor;
        view.opaque          = NO;
    }
}

static void LTable_UpdateCell(UITableView* table, UITableViewCell* cell, int row) {
    struct ServerInfo* server = LTable_Get(row);
    struct Flag* flag = Flags_Get(server);
    
    char descBuffer[128];
    cc_string desc = String_FromArray(descBuffer);
    String_Format2(&desc, "%i/%i players, up for ", &server->players, &server->maxPlayers);
    LTable_FormatUptime(&desc, server->uptime);
    if (server->software.length) String_Format1(&desc, " | %s", &server->software);
    
    if (flag && flag->meta)
        cell.imageView.image = (__bridge UIImage*)flag->meta;
        
    cell.textLabel.text       = ToNSString(&server->name);
    cell.detailTextLabel.text = ToNSString(&desc);//[ToNSString(&desc) stringByAppendingString:@"\nLine2"];
    cell.textLabel.textColor  = UIColor.whiteColor;
    cell.detailTextLabel.textColor = UIColor.whiteColor;
    cell.selectionStyle       = UITableViewCellSelectionStyleNone;
    
    NSIndexPath* sel = table.indexPathForSelectedRow;
    cc_bool selected = sel && sel.row == row;
    LTable_UpdateCellColor(cell, server, row, selected);
}

// TODO only redraw flags
static void OnFlagsChanged(void) {
	struct LScreen* s = Launcher_Active;
    for (int i = 0; i < s->numWidgets; i++)
    {
		if (s->widgets[i]->type != LWIDGET_TABLE) continue;
        UITableView* tbl = (__bridge UITableView*)s->widgets[i]->meta;
    
		// trying to update cell.imageView.image doesn't seem to work,
		// so pointlessly reload entire table data instead
		NSIndexPath* selected = [tbl indexPathForSelectedRow];
		[tbl reloadData];
		[tbl selectRowAtIndexPath:selected animated:NO scrollPosition:UITableViewScrollPositionNone];
    }
}

/*########################################################################################################################*
 *------------------------------------------------------UI Backend--------------------------------------------------------*
 *#########################################################################################################################*/
void LBackend_DecodeFlag(struct Flag* flag, cc_uint8* data, cc_uint32 len) {
	NSData* ns_data = [NSData dataWithBytes:data length:len];
	UIImage* img = [UIImage imageWithData:ns_data];
	if (!img) return;
	
    flag->meta = CFBridgingRetain(img);  
	OnFlagsChanged();
}

static void LBackend_LayoutDimensions(struct LWidget* w, CGRect* r) {
    const struct LLayout* l = w->layouts + 2;
    while (l->type)
    {
        switch (l->type)
        {
            case LLAYOUT_WIDTH:
                r->size.width  = Window_Main.Width  - (int)r->origin.x - Display_ScaleX(l->offset);
                break;
            case LLAYOUT_HEIGHT:
                r->size.height = Window_Main.Height - (int)r->origin.y - Display_ScaleY(l->offset);
                break;
        }
        l++;
    }
}

void LBackend_LayoutWidget(struct LWidget* w) {
    const struct LLayout* l = w->layouts;
    UIView* view = (__bridge UIView*)w->meta;
    CGRect r     = [view frame];
    int width    = (int)r.size.width;
    int height   = (int)r.size.height;
    
    r.origin.x = Gui_CalcPos(l[0].type & 0xFF, Display_ScaleX(l[0].offset), width,  Window_Main.Width);
    r.origin.y = Gui_CalcPos(l[1].type & 0xFF, Display_ScaleY(l[1].offset), height, Window_Main.Height);
    
    // e.g. Table widget needs adjusts width/height based on window
    if (l[1].type & LLAYOUT_EXTRA)
        LBackend_LayoutDimensions(w, &r);
    view.frame = r;
}

static UIView* ShowWidget(struct LWidget* w) {
    switch (w->type)
    {
        case LWIDGET_BUTTON:
            return LBackend_ButtonShow((struct LButton*)w);
        case LWIDGET_CHECKBOX:
            return LBackend_CheckboxShow((struct LCheckbox*)w);
        case LWIDGET_INPUT:
            return LBackend_InputShow((struct LInput*)w);
        case LWIDGET_LABEL:
            return LBackend_LabelShow((struct LLabel*)w);
        case LWIDGET_LINE:
            return LBackend_LineShow((struct LLine*)w);
        case LWIDGET_SLIDER:
            return LBackend_SliderShow((struct LSlider*)w);
        case LWIDGET_TABLE:
            return LBackend_TableShow((struct LTable*)w);
    }
    return NULL;
}

void LBackend_SetScreen(struct LScreen* s) {
    for (int i = 0; i < s->numWidgets; i++)
    {
        struct LWidget* w = s->widgets[i];
        UIView* view      = ShowWidget(w);
        
        [view_handle addSubview:view];
    }
    // TODO replace with native constraints some day, maybe
    s->Layout(s);
}

void LBackend_CloseScreen(struct LScreen* s) {
    if (!s) return;
    
    // remove reference to soon to be garbage collected views
    for (int i = 0; i < s->numWidgets; i++)
    {
        s->widgets[i]->meta = NULL;
    }
    
    // remove all widgets from previous screen
    NSArray* elems = [view_handle subviews];
    for (UIView* view in elems)
    {
        [view removeFromSuperview];
    }
}
#endif
