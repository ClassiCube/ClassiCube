#ifndef CS_OPTIONS_H
#define CS_OPTIONS_H
#include "Typedefs.h"
#include "String.h"
/* Manages loading and saving options.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

typedef UInt8 FpsLimitMethod;
#define FpsLimitMethod_VSync  0
#define FpsLimitMethod_30FPS  1
#define FpsLimitMethod_60FPS  2
#define FpsLimitMethod_120FPS 3
#define FpsLimitMethod_None   4

#define OptionsKey_UseMusic "usemusic"
#define OptionsKey_UseSound "usesound"
#define OptionsKey_MusicVolume "musicvolume"
#define OptionsKey_SoundVolume "soundvolume"
#define OptionsKey_ForceOpenAL "forceopenal"
#define OptionsKey_ForceOldOpenGL "force-oldgl"

#define OptionsKey_ViewDist "viewdist"
#define OptionsKey_BlockPhysics "singleplayerphysics"
#define OptionsKey_NamesMode "namesmode"
#define OptionsKey_InvertMouse "invertmouse"
#define OptionsKey_Sensitivity "mousesensitivity"
#define OptionsKey_FpsLimit "fpslimit"
#define OptionsKey_DefaultTexturePack "defaulttexpack"
#define OptionsKey_AutoCloseLauncher "autocloselauncher"
#define OptionsKey_ViewBobbing "viewbobbing"
#define OptionsKey_EntityShadow "entityshadow"
#define OptionsKey_RenderType "normal"
#define OptionsKey_SmoothLighting "gfx-smoothlighting"
#define OptionsKey_Mipmaps "gfx-mipmaps"
#define OptionsKey_SurvivalMode "game-survivalmode"
#define OptionsKey_ChatLogging "chat-logging"

#define OptionsKey_HacksEnabled "hacks-hacksenabled"
#define OptionsKey_FieldOfView "hacks-fov"
#define OptionsKey_Speed "hacks-speedmultiplier"
#define OptionsKey_ModifiableLiquids "hacks-liquidsbreakable"
#define OptionsKey_PushbackPlacing "hacks-pushbackplacing"
#define OptionsKey_NoclipSlide "hacks-noclipslide"
#define OptionsKey_CameraClipping "hacks-cameraclipping"
#define OptionsKey_WOMStyleHacks "hacks-womstylehacks"
#define OptionsKey_FullBlockStep "hacks-fullblockstep"

#define OptionsKey_TabAutocomplete "gui-tab-autocomplete"
#define OptionsKey_ShowBlockInHand "gui-blockinhand"
#define OptionsKey_ChatLines "gui-chatlines"
#define OptionsKey_ClickableChat "gui-chatclickable"
#define OptionsKey_UseChatFont "gui-arialchatfont"
#define OptionsKey_HotbarScale "gui-hotbarscale"
#define OptionsKey_InventoryScale "gui-inventoryscale"
#define OptionsKey_ChatScale "gui-chatscale"
#define OptionsKey_ShowFPS "gui-showfps"
#define OptionsKey_FontName "gui-fontname"
#define OptionsKey_BlackText "gui-blacktextshadows"

#define OptionsKey_UseCustomBlocks "nostalgia-customblocks"
#define OptionsKey_UseCPE "nostalgia-usecpe"
#define OptionsKey_UseServerTextures "nostalgia-servertextures"
#define OptionsKey_UseClassicGui "nostalgia-classicgui"
#define OptionsKey_SimpleArmsAnim "nostalgia-simplearms"
#define OptionsKey_UseClassicTabList "nostalgia-classictablist"
#define OptionsKey_UseClassicOptions "nostalgia-classicoptions"
#define OptionsKey_AllowClassicHacks "nostalgia-hacks"
#define OptionsKey_ClassicArmModel "nostalgia-classicarm"

#define OPTIONS_LARGESTRS 4
#define OPTIONS_MEDSTRS 16
#define OPTIONS_SMALLSTRS 32
#define OPTIONS_TINYSTRS 64
#define OPTIONS_COUNT (OPTIONS_LARGESTRS + OPTIONS_MEDSTRS + OPTIONS_SMALLSTRS + OPTIONS_TINYSTRS)

String Options_Keys[OPTIONS_COUNT];
String Options_Values[OPTIONS_COUNT];
bool Options_Changed[OPTIONS_COUNT];

void Options_Init(void);

String Options_Get(const UInt8* key);
Int32 Options_GetInt(const UInt8* key, Int32 min, Int32 max, Int32 defValue);
bool Options_GetBool(const UInt8* key, bool defValue);
Real32 Options_GetFloat(const UInt8* key, Real32 min, Real32 max, Real32 defValue);

void Options_SetInt32(const UInt8* keyRaw, Int32 value);
void Options_Set(const UInt8* keyRaw, STRING_TRANSIENT String value);
#endif