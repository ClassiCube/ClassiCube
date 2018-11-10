#ifndef CC_MENUS_H
#define CC_MENUS_H
#include "String.h"

/* Contains all 2D menu screen implementations.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct Screen;

struct Screen* PauseScreen_MakeInstance(void);
struct Screen* OptionsGroupScreen_MakeInstance(void);
struct Screen* ClassicOptionsScreen_MakeInstance(void);

struct Screen* ClassicKeyBindingsScreen_MakeInstance(void);
struct Screen* ClassicHacksKeyBindingsScreen_MakeInstance(void);
struct Screen* NormalKeyBindingsScreen_MakeInstance(void);
struct Screen* HacksKeyBindingsScreen_MakeInstance(void);
struct Screen* OtherKeyBindingsScreen_MakeInstance(void);
struct Screen* MouseKeyBindingsScreen_MakeInstance(void);

struct Screen* GenLevelScreen_MakeInstance(void);
struct Screen* ClassicGenScreen_MakeInstance(void);
struct Screen* LoadLevelScreen_MakeInstance(void);
struct Screen* SaveLevelScreen_MakeInstance(void);
struct Screen* TexturePackScreen_MakeInstance(void);
struct Screen* FontListScreen_MakeInstance(void);
struct Screen* HotkeyListScreen_MakeInstance(void);

struct Screen* MiscOptionsScreen_MakeInstance(void);
struct Screen* GuiOptionsScreen_MakeInstance(void);
struct Screen* GraphicsOptionsScreen_MakeInstance(void);
struct Screen* HacksSettingsScreen_MakeInstance(void);
struct Screen* EnvSettingsScreen_MakeInstance(void);
struct Screen* NostalgiaScreen_MakeInstance(void);

struct Screen* UrlWarningOverlay_MakeInstance(const String* url);
struct Screen* TexIdsOverlay_MakeInstance(void);
struct Screen* TexPackOverlay_MakeInstance(const String* url);
#endif
