#ifndef CS_OPTIONS_H
#define CS_OPTIONS_H
#include "Typedefs.h"
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
#define ForceOldOpenGL "force-oldgl"

#define OptionsKey_ViewDist "viewdist"
#define OptionsKey_SingleplayerPhysics "singleplayerphysics"
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
#define OptionsKey_ArialChatFont "gui-arialchatfont"
#define OptionsKey_HotbarScale "gui-hotbarscale"
#define OptionsKey_InventoryScale "gui-inventoryscale"
#define OptionsKey_ChatScale "gui-chatscale"
#define OptionsKey_ShowFPS "gui-showfps"
#define OptionsKey_FontName "gui-fontname"
#define OptionsKey_BlackTextShadows "gui-blacktextshadows"

#define OptionsKey_AllowCustomBlocks "nostalgia-customblocks"
#define OptionsKey_UseCPE "nostalgia-usecpe"
#define OptionsKey_AllowServerTextures "nostalgia-servertextures"
#define OptionsKey_UseClassicGui "nostalgia-classicgui"
#define OptionsKey_SimpleArmsAnim "nostalgia-simplearms"
#define OptionsKey_UseClassicTabList "nostalgia-classictablist"
#define OptionsKey_UseClassicOptions "nostalgia-classicoptions"
#define OptionsKey_AllowClassicHacks "nostalgia-hacks"
#endif