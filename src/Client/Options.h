#ifndef CC_OPTIONS_H
#define CC_OPTIONS_H
#include "Typedefs.h"
#include "String.h"
/* Manages loading and saving options.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

#define FPS_LIMIT_VSYNC  0
#define FPS_LIMIT_30FPS  1
#define FPS_LIMIT_60FPS  2
#define FPS_LIMIT_120FPS 3
#define FPS_LIMIT_NONE   4

#define OPTION_USE_MUSIC "usemusic"
#define OPTION_USE_SOUND "usesound"
#define OPTION_MUSIC_VOLUME "musicvolume"
#define OPTION_SOUND_VOLUME "soundvolume"
#define OPTION_FORCE_OPENAL "forceopenal"
#define OPTION_FORCE_OLD_OPENGL "force-oldgl"

#define OPTION_VIEW_DISTANCE "viewdist"
#define OPTION_BLOCK_PHYSICS "singleplayerphysics"
#define OPTION_NAMES_MODE "namesmode"
#define OPTION_INVERT_MOUSE "invertmouse"
#define OPTION_SENSITIVITY "mousesensitivity"
#define OPTION_FPS_LIMIT "fpslimit"
#define OPTION_DEFAULT_TEX_PACK "defaulttexpack"
#define OPTION_AUTO_CLOSE_LAUNCHER "autocloselauncher"
#define OPTION_VIEW_BOBBING "viewbobbing"
#define OPTION_ENTITY_SHADOW "entityshadow"
#define OPTION_RENDER_TYPE "normal"
#define OPTION_SMOOTH_LIGHTING "gfx-smoothlighting"
#define OPTION_MIPMAPS "gfx-mipmaps"
#define OPTION_SURVIVAL_MODE "game-survivalmode"
#define OPTION_CHAT_LOGGING "chat-logging"
#define OPTION_WINDOW_WIDTH "window-width"
#define OPTION_WINDOW_HEIGHT "window-height"

#define OPTION_HACKS_ENABLED "hacks-hacksenabled"
#define OPTION_FIELD_OF_VIEW "hacks-fov"
#define OPTION_SPEED_FACTOR "hacks-speedmultiplier"
#define OPTION_JUMP_VELOCITY "hacks-jumpvelocity"
#define OPTION_MODIFIABLE_LIQUIDS "hacks-liquidsbreakable"
#define OPTION_PUSHBACK_PLACING "hacks-pushbackplacing"
#define OPTION_NOCLIP_SLIDE "hacks-noclipslide"
#define OPTION_CAMERA_CLIPPING "hacks-cameraclipping"
#define OPTION_WOM_STYLE_HACKS "hacks-womstylehacks"
#define OPTION_FULL_BLOCK_STEP "hacks-fullblockstep"

#define OPTION_TAB_AUTOCOMPLETE "gui-tab-autocomplete"
#define OPTION_SHOW_BLOCK_IN_HAND "gui-blockinhand"
#define OPTION_CHATLINES "gui-chatlines"
#define OPTION_CLICKABLE_CHAT "gui-chatclickable"
#define OPTION_USE_CHAT_FONT "gui-arialchatfont"
#define OPTION_HOTBAR_SCALE "gui-hotbarscale"
#define OPTION_INVENTORY_SCALE "gui-inventoryscale"
#define OPTION_CHAT_SCALE "gui-chatscale"
#define OPTION_SHOW_FPS "gui-showfps"
#define OPTION_FONT_NAME "gui-fontname"
#define OPTION_BLACK_TEXT "gui-blacktextshadows"

#define OPTION_USE_CUSTOM_BLOCKS "nostalgia-customblocks"
#define OPTION_USE_CPE "nostalgia-usecpe"
#define OPTION_USE_SERVER_TEXTURES "nostalgia-servertextures"
#define OPTION_USE_CLASSIC_GUI "nostalgia-classicgui"
#define OPTION_SIMPLE_ARMS_ANIM "nostalgia-simplearms"
#define OPTION_USE_CLASSIC_TABLIST "nostalgia-classictablist"
#define OPTION_USE_CLASSIC_OPTIONS "nostalgia-classicoptions"
#define OPTION_ALLOW_CLASSIC_HACKS "nostalgia-hacks"
#define OPTION_CLASSIC_ARM_MODEL "nostalgia-classicarm"

StringsBuffer Options_Keys;
StringsBuffer Options_Values;

void Options_Init(void);
void Options_Free(void);

void Options_Get(const UInt8* key, STRING_TRANSIENT String* value);
Int32 Options_GetInt(const UInt8* key, Int32 min, Int32 max, Int32 defValue);
bool Options_GetBool(const UInt8* key, bool defValue);
Real32 Options_GetFloat(const UInt8* key, Real32 min, Real32 max, Real32 defValue);
UInt32 Options_GetEnum(const UInt8* key, UInt32 defValue, const UInt8** names, UInt32 namesCount);

void Options_SetInt32(const UInt8* keyRaw, Int32 value);
void Options_Set(const UInt8* keyRaw, STRING_PURE String* value);
void Options_Load(void);
void Options_Save(void);
#endif