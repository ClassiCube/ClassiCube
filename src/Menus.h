#ifndef CC_MENUS_H
#define CC_MENUS_H
#include "Gui.h"
#include "PackedCol.h"
CC_BEGIN_HEADER


/* Contains all 2D menu screen implementations.
   Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/
struct Screen;
struct MenuInputDesc;
struct FontDesc;
struct ButtonWidget;
struct InputDevice;
struct MenuOptionsScreen;

int Menu_InputDown(void* screen, int key, struct InputDevice* device);
int Menu_PointerDown(void* screen, int id, int x, int y);
int Menu_PointerMove(void* screen, int id, int x, int y);
int Menu_DoPointerMove(void* screen, int id, int x, int y);

struct SimpleButtonDesc { short x, y; const char* title; Widget_LeftClick onClick; };
void Menu_AddButtons(void* screen, struct ButtonWidget* btns, int width, 
					const struct SimpleButtonDesc* descs, int count);
void Menu_LayoutButtons(struct ButtonWidget* btns, 
					const struct SimpleButtonDesc* descs, int count);
void Menu_SetButtons(struct ButtonWidget* btns, struct FontDesc* font, 
					const struct SimpleButtonDesc* descs, int count);
void Menu_LayoutBack(struct ButtonWidget* btn);

void PauseScreen_Show(void);
void OptionsGroupScreen_Show(void);
void ClassicOptionsScreen_Show(void);
void ClassicPauseScreen_Show(void);

void ClassicBindingsScreen_Show(void);
void ClassicHacksBindingsScreen_Show(void);
void NormalBindingsScreen_Show(void);
void HacksBindingsScreen_Show(void);
void OtherBindingsScreen_Show(void);
void MouseBindingsScreen_Show(void);
void HotbarBindingsScreen_Show(void);

void GenLevelScreen_Show(void);
void ClassicGenScreen_Show(void);
void LoadLevelScreen_Show(void);
void SaveLevelScreen_Show(void);
void TexturePackScreen_Show(void);
void FontListScreen_Show(void);
void HotkeyListScreen_Show(void);

void MiscOptionsScreen_Show(void);
void ChatOptionsScreen_Show(void);
void GuiOptionsScreen_Show(void);
void GraphicsOptionsScreen_Show(void);
void HacksSettingsScreen_Show(void);
void EnvSettingsScreen_Show(void);
void NostalgiaAppearanceScreen_Show(void);
void NostalgiaFunctionalityScreen_Show(void);
void NostalgiaMenuScreen_Show(void);

void UrlWarningOverlay_Show(const cc_string* url);
void TexIdsOverlay_Show(void);
void TexPackOverlay_Show(const cc_string* url);
#ifdef CC_BUILD_TOUCH
void TouchCtrlsScreen_Show(void);
void TouchMoreScreen_Show(void);
void TouchOnscreenScreen_Show(void);
#endif

void MenuScreen_Render2(void* screen, float delta);
typedef void (*MenuInputDone)(const cc_string* value, cc_bool valid);
void MenuInputOverlay_Show(struct MenuInputDesc* desc, const cc_string* value, MenuInputDone onDone, cc_bool screenMode);
void MenuInputOverlay_Close(cc_bool valid);


typedef cc_bool (*Button_GetBool)(void);
typedef void    (*Button_SetBool)(cc_bool value);
void MenuOptionsScreen_AddBool(struct MenuOptionsScreen* s, const char* name, 
								Button_GetBool getValue, Button_SetBool setValue, const char* desc);

typedef int  (*Button_GetEnum)(void);
typedef void (*Button_SetEnum)(int value);
void MenuOptionsScreen_AddEnum(struct MenuOptionsScreen* s, const char* name,
								const char* const* names, int namesCount,
								Button_GetEnum getValue, Button_SetEnum setValue, const char* desc);

typedef PackedCol (*Button_GetHex)(void);
typedef void      (*Button_SetHex)(PackedCol value);
void MenuOptionsScreen_AddHex(struct MenuOptionsScreen* s, const char* name, PackedCol defaultValue,
								Button_GetHex getValue, Button_SetHex setValue, const char* desc);

typedef int  (*Button_GetInt)(void);
typedef void (*Button_SetInt)(int value);
void MenuOptionsScreen_AddInt(struct MenuOptionsScreen* s, const char* name,
								int minValue, int maxValue, int defaultValue,
								Button_GetInt getValue, Button_SetInt setValue, const char* desc);

typedef void (*Button_GetNum)(cc_string* v);
typedef void (*Button_SetNum)(const cc_string* v);
void MenuOptionsScreen_AddNum(struct MenuOptionsScreen* s, const char* name,
								float minValue, float maxValue, float defaultValue,
								Button_GetNum getValue, Button_SetNum setValue, const char* desc);

CC_END_HEADER
#endif
