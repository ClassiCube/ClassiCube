#ifndef CC_MENUS_H
#define CC_MENUS_H
#include "Gui.h"

/* Contains all 2D menu screen implementations.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

Screen* PauseScreen_MakeInstance(void);
Screen* OptionsGroupScreen_MakeInstance(void);
Screen* GenLevelScree_MakeInstance(void);
Screen* ClassicGenLevelScreen_MakeInstance(void);
Screen* LoadLevelScreen_MakeInstance(void);
Screen* SaveLevelScreen_MakeInstance(void);
Screen* TexturePackScreen_MakeInstance(void);
Screen* HotkeyListScreen_MakeInstance(void);
Screen* NostalgiaScreen_MakeInstance(void);
Screen* ClassicOptionsScreen_MakeInstance(void);
#endif