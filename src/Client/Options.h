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
	
const UInt8* OptionsKey_ViewDist = "viewdist";
const UInt8* OptionsKey_SingleplayerPhysics = "singleplayerphysics";
const UInt8* OptionsKey_UseMusic = "usemusic";
const UInt8* OptionsKey_UseSound = "usesound";
const UInt8* OptionsKey_ForceOpenAL = "forceopenal";
const UInt8* OptionsKey_NamesMode = "namesmode";
const UInt8* OptionsKey_InvertMouse = "invertmouse";
const UInt8* OptionsKey_Sensitivity = "mousesensitivity";
const UInt8* OptionsKey_FpsLimit = "fpslimit";
const UInt8* OptionsKey_DefaultTexturePack = "defaulttexpack";
const UInt8* OptionsKey_AutoCloseLauncher = "autocloselauncher";
const UInt8* OptionsKey_ViewBobbing = "viewbobbing";
const UInt8* OptionsKey_EntityShadow = "entityshadow";
const UInt8* OptionsKey_RenderType = "normal";
const UInt8* OptionsKey_SmoothLighting = "gfx-smoothlighting";
const UInt8* OptionsKey_SurvivalMode = "game-survivalmode";
const UInt8* OptionsKey_ChatLogging = "chat-logging";

const UInt8* OptionsKey_HacksEnabled = "hacks-hacksenabled";
const UInt8* OptionsKey_FieldOfView = "hacks-fov";
const UInt8* OptionsKey_Speed = "hacks-speedmultiplier";
const UInt8* OptionsKey_ModifiableLiquids = "hacks-liquidsbreakable";
const UInt8* OptionsKey_PushbackPlacing = "hacks-pushbackplacing";
const UInt8* OptionsKey_NoclipSlide = "hacks-noclipslide";
const UInt8* OptionsKey_CameraClipping = "hacks-cameraclipping";
const UInt8* OptionsKey_WOMStyleHacks = "hacks-womstylehacks";
const UInt8* OptionsKey_FullBlockStep = "hacks-fullblockstep";

const UInt8* OptionsKey_TabAutocomplete = "gui-tab-autocomplete";
const UInt8* OptionsKey_ShowBlockInHand = "gui-blockinhand";
const UInt8* OptionsKey_ChatLines = "gui-chatlines";
const UInt8* OptionsKey_ClickableChat = "gui-chatclickable";
const UInt8* OptionsKey_ArialChatFont = "gui-arialchatfont";
const UInt8* OptionsKey_HotbarScale = "gui-hotbarscale";
const UInt8* OptionsKey_InventoryScale = "gui-inventoryscale";
const UInt8* OptionsKey_ChatScale = "gui-chatscale";
const UInt8* OptionsKey_ShowFPS = "gui-showfps";
const UInt8* OptionsKey_FontName = "gui-fontname";
const UInt8* OptionsKey_BlackTextShadows = "gui-blacktextshadows";

const UInt8* OptionsKey_AllowCustomBlocks = "nostalgia-customblocks";
const UInt8* OptionsKey_UseCPE = "nostalgia-usecpe";
const UInt8* OptionsKey_AllowServerTextures = "nostalgia-servertextures";
const UInt8* OptionsKey_UseClassicGui = "nostalgia-classicgui";
const UInt8* OptionsKey_SimpleArmsAnim = "nostalgia-simplearms";
const UInt8* OptionsKey_UseClassicTabList = "nostalgia-classictablist";
const UInt8* OptionsKey_UseClassicOptions = "nostalgia-classicoptions";
const UInt8* OptionsKey_AllowClassicHacks = "nostalgia-hacks";
#endif