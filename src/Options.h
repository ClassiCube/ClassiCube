#ifndef CC_OPTIONS_H
#define CC_OPTIONS_H
#include "Core.h"
/* Manages loading and saving options.
   Copyright 2014-2021 ClassiCube | Licensed under BSD-3
*/

#define OPT_USE_MUSIC "usemusic"
#define OPT_USE_SOUND "usesound"
#define OPT_MUSIC_VOLUME "musicvolume"
#define OPT_SOUND_VOLUME "soundsvolume"
#define OPT_FORCE_OPENAL "forceopenal"
#define OPT_MIN_MUSIC_DELAY "music-mindelay"
#define OPT_MAX_MUSIC_DELAY "music-maxdelay"

#define OPT_VIEW_DISTANCE "viewdist"
#define OPT_BLOCK_PHYSICS "singleplayerphysics"
#define OPT_NAMES_MODE "namesmode"
#define OPT_INVERT_MOUSE "invertmouse"
#define OPT_SENSITIVITY "mousesensitivity"
#define OPT_FPS_LIMIT "fpslimit"
#define OPT_DEFAULT_TEX_PACK "defaulttexpack"
#define OPT_AUTO_CLOSE_LAUNCHER "autocloselauncher"
#define OPT_VIEW_BOBBING "viewbobbing"
#define OPT_ENTITY_SHADOW "entityshadow"
#define OPT_RENDER_TYPE "normal"
#define OPT_SMOOTH_LIGHTING "gfx-smoothlighting"
#define OPT_MIPMAPS "gfx-mipmaps"
#define OPT_CHAT_LOGGING "chat-logging"
#define OPT_WINDOW_WIDTH "window-width"
#define OPT_WINDOW_HEIGHT "window-height"

#define OPT_HACKS_ENABLED "hacks-hacksenabled"
#define OPT_FIELD_OF_VIEW "hacks-fov"
#define OPT_SPEED_FACTOR "hacks-speedmultiplier"
#define OPT_JUMP_VELOCITY "hacks-jumpvelocity"
#define OPT_MODIFIABLE_LIQUIDS "hacks-liquidsbreakable"
#define OPT_PUSHBACK_PLACING "hacks-pushbackplacing"
#define OPT_NOCLIP_SLIDE "hacks-noclipslide"
#define OPT_CAMERA_CLIPPING "hacks-cameraclipping"
#define OPT_WOM_STYLE_HACKS "hacks-womstylehacks"
#define OPT_FULL_BLOCK_STEP "hacks-fullblockstep"
#define OPT_HACK_PERM_MSGS "hacks-perm-msgs"

#define OPT_TAB_AUTOCOMPLETE "gui-tab-autocomplete"
#define OPT_SHOW_BLOCK_IN_HAND "gui-blockinhand"
#define OPT_CHATLINES "gui-chatlines"
#define OPT_CLICKABLE_CHAT "gui-chatclickable"
#define OPT_USE_CHAT_FONT "gui-arialchatfont"
#define OPT_HOTBAR_SCALE "gui-hotbarscale"
#define OPT_INVENTORY_SCALE "gui-inventoryscale"
#define OPT_CHAT_SCALE "gui-chatscale"
#define OPT_SHOW_FPS "gui-showfps"
#define OPT_FONT_NAME "gui-fontname"
#define OPT_BLACK_TEXT "gui-blacktextshadows"

#define OPT_CLASSIC_MODE "mode-classic"
#define OPT_CUSTOM_BLOCKS "nostalgia-customblocks"
#define OPT_CPE "nostalgia-usecpe"
#define OPT_SERVER_TEXTURES "nostalgia-servertextures"
#define OPT_CLASSIC_GUI "nostalgia-classicgui"
#define OPT_SIMPLE_ARMS_ANIM "nostalgia-simplearms"
#define OPT_CLASSIC_TABLIST "nostalgia-classictablist"
#define OPT_CLASSIC_OPTIONS "nostalgia-classicoptions"
#define OPT_CLASSIC_HACKS "nostalgia-hacks"
#define OPT_CLASSIC_ARM_MODEL "nostalgia-classicarm"
#define OPT_CLASSIC_CHAT "nostalgia-classicchat"
#define OPT_MAX_CHUNK_UPDATES "gfx-maxchunkupdates"
#define OPT_CAMERA_MASS "cameramass"
#define OPT_CAMERA_SMOOTH "camera-smooth"
#define OPT_GRAB_CURSOR "win-grab-cursor"
#define OPT_TOUCH_BUTTONS "gui-touchbuttons"
#define OPT_TOUCH_SCALE "gui-touchscale"
#define OPT_HTTP_ONLY "http-no-https"
#define OPT_RAW_INPUT "win-raw-input"

#define LOPT_SESSION  "launcher-session"
#define LOPT_USERNAME "launcher-cc-username"
#define LOPT_PASSWORD "launcher-cc-password"

#define ROPT_SERVER "launcher-server"
#define ROPT_USER   "launcher-username"
#define ROPT_IP     "launcher-ip"
#define ROPT_PORT   "launcher-port"
#define ROPT_MPPASS "launcher-mppass"

#define SOPT_SERVICES "server-services"

struct StringsBuffer;
extern struct StringsBuffer Options;
extern cc_result Options_LoadResult;
/* Frees any memory allocated in storing options. */
void Options_Free(void);

/* Loads options from disc. */
void Options_Load(void);
/* Reloads options from disc, leaving options changed in this session alone. */
CC_API void Options_Reload(void);
/* Saves options to disc, if any were changed via Options_SetXYZ since last save. */
CC_API void Options_SaveIfChanged(void);

/* Sets value to value of option directly in Options.Buffer if found, String_Empty if not. */
/* Returns whether the option was actually found. */
STRING_REF cc_bool Options_UNSAFE_Get(const char* keyRaw, cc_string* value);
/* Returns value of given option, or defalt value if not found. */
CC_API void Options_Get(const char*        key, cc_string* value, const char* defValue);
/* Returns value of given option as an integer, or defalt value if could not be converted. */
CC_API int  Options_GetInt(const char*     key, int min, int max, int defValue);
/* Returns value of given option as a bool, or defalt value if could not be converted. */
CC_API cc_bool Options_GetBool(const char* key, cc_bool defValue);
/* Returns value of given option as a float, or defalt value if could not be converted. */
CC_API float Options_GetFloat(const char*  key, float min, float max, float defValue);
/* Returns value of given option as an integer, or defalt value if could not be converted. */
/* NOTE: Conversion is done by going through all elements of names, returning index of a match. */
CC_API int   Options_GetEnum(const char*   key, int defValue, const char* const* names, int namesCount);

/* Sets value of given option to either "true" or "false". */
CC_API void Options_SetBool(const char* keyRaw, cc_bool value);
/* Sets value of given option to given integer converted to a string. */
CC_API void Options_SetInt(const char*  keyRaw,  int value);
/* Sets value of given option to given string. */
CC_API void Options_Set(const char*     keyRaw,  const cc_string* value);
/* Sets value of given option to given string. */
CC_API void Options_SetString(const cc_string* key, const cc_string* value);

/* Attempts to securely encode an option. */
/* NOTE: Not all platforms support secure saving. DO NOT RELY ON THIS BEING SECURE! */
void Options_SetSecure(const char* opt, const cc_string* data, const cc_string* key);
/* Attempts to securely decode an option. */
/* NOTE: Not all platforms support secure saving. DO NOT RELY ON THIS BEING SECURE! */
void Options_GetSecure(const char* opt, cc_string* data, const cc_string* key);
#endif
