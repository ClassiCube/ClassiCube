#ifndef CC_MENUS_H
#define CC_MENUS_H
#include "String.h"

/* Contains all 2D menu screen implementations.
   Copyright 2014-2019 ClassiCube | Licensed under BSD-3
*/
struct Screen;

void PauseScreen_Show(void);
void OptionsGroupScreen_Show(void);
struct Screen* ClassicOptionsScreen_MakeInstance(void);

void ClassicKeyBindingsScreen_Show(void);
void ClassicHacksKeyBindingsScreen_Show(void);
void NormalKeyBindingsScreen_Show(void);
void HacksKeyBindingsScreen_Show(void);
void OtherKeyBindingsScreen_Show(void);
void MouseKeyBindingsScreen_Show(void);

void GenLevelScreen_Show(void);
void ClassicGenScreen_Show(void);
void LoadLevelScreen_Show(void);
void SaveLevelScreen_Show(void);
void TexturePackScreen_Show(void);
void FontListScreen_Show(void);
void HotkeyListScreen_Show(void);

struct Screen* MiscOptionsScreen_MakeInstance(void);
struct Screen* GuiOptionsScreen_MakeInstance(void);
struct Screen* GraphicsOptionsScreen_MakeInstance(void);
struct Screen* HacksSettingsScreen_MakeInstance(void);
struct Screen* EnvSettingsScreen_MakeInstance(void);
struct Screen* NostalgiaScreen_MakeInstance(void);

void UrlWarningOverlay_Show(const String* url);
void TexIdsOverlay_Show(void);
void TexPackOverlay_Show(const String* url);
#endif
