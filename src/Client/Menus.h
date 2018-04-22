#ifndef CC_MENUS_H
#define CC_MENUS_H
#include "Gui.h"

/* Contains all 2D menu screen implementations.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

Screen* PauseScreen_MakeInstance(void);
Screen* OptionsGroupScreen_MakeInstance(void);
Screen* ClassicOptionsScreen_MakeInstance(void);

Screen* ClassicKeyBindingsScreen_MakeInstance(void);
Screen* ClassicHacksKeyBindingsScreen_MakeInstance(void);
Screen* NormalKeyBindingsScreen_MakeInstance(void);
Screen* HacksKeyBindingsScreen_MakeInstance(void);
Screen* OtherKeyBindingsScreen_MakeInstance(void);
Screen* MouseKeyBindingsScreen_MakeInstance(void);

Screen* GenLevelScreen_MakeInstance(void);
Screen* ClassicGenScreen_MakeInstance(void);
Screen* LoadLevelScreen_MakeInstance(void);
Screen* SaveLevelScreen_MakeInstance(void);
Screen* TexturePackScreen_MakeInstance(void);
Screen* HotkeyListScreen_MakeInstance(void);

Screen* MiscOptionsScreen_MakeInstance(void);
Screen* GuiOptionsScreen_MakeInstance(void);
Screen* GraphicsOptionsScreen_MakeInstance(void);
Screen* HacksSettingsScreen_MakeInstance(void);
Screen* EnvSettingsScreen_MakeInstance(void);
Screen* NostalgiaScreen_MakeInstance(void);

Screen* UrlWarningOverlay_MakeInstance(STRING_PURE String* url);
Screen* TexIdsOverlay_MakeInstance(void);
Screen* TexPackOverlay_MakeInstance(STRING_PURE String* url);
#endif