#ifndef CC_MENUS_H
#define CC_MENUS_H
#include "Core.h"

/* Contains all 2D menu screen implementations.
   Copyright 2014-2022 ClassiCube | Licensed under BSD-3
*/
struct Screen;
int Menu_PointerDown(void* screen, int id, int x, int y);
int Menu_PointerMove(void* screen, int id, int x, int y);

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
#endif
