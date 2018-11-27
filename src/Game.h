#ifndef CC_GAME_H
#define CC_GAME_H
#include "Picking.h"
#include "Options.h"
#include "Bitmap.h"
/* Represents the game.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

struct DisplayDevice;
struct Stream;

extern int Game_Width, Game_Height;
/* Total rendering time (in seconds) elapsed since the client was started. */
extern double Game_Accumulator;
extern int Game_ChunkUpdates;
extern bool Game_CameraClipping;
extern struct PickedPos Game_SelectedPos;
extern struct PickedPos Game_CameraClipPos;
extern bool Game_UseCPEBlocks;

extern String Game_Username;
extern String Game_Mppass;
extern String Game_IPAddress;
extern int    Game_Port;

extern int Game_ViewDistance;
extern int Game_MaxViewDistance;
extern int Game_UserViewDistance;
extern int Game_Fov;
extern int Game_DefaultFov, Game_ZoomFov;

extern float game_limitMs;
extern int  Game_FpsLimit;
extern bool Game_ShowAxisLines;
extern bool Game_SimpleArmsAnim;
extern bool Game_ClassicArmModel;
extern bool Game_InvertMouse;
extern int  Game_Vertices;

extern int  Game_MouseSensitivity;
extern bool Game_TabAutocomplete;
extern bool Game_UseClassicGui;
extern bool Game_UseClassicTabList;
extern bool Game_UseClassicOptions;
extern bool Game_ClassicMode;
extern bool Game_ClassicHacks;
#define Game_PureClassic (Game_ClassicMode && !Game_ClassicHacks)
extern bool Game_AllowCustomBlocks;
extern bool Game_UseCPE;
extern bool Game_AllowServerTextures;
extern bool Game_SmoothLighting;
extern bool Game_ChatLogging;
extern bool Game_AutoRotate;
extern bool Game_SmoothCamera;
extern String Game_FontName;
extern int  Game_ChatLines;
extern bool Game_ClickableChat;
extern bool Game_HideGui;
extern bool Game_ShowFPS;

extern bool Game_ViewBobbing;
extern bool Game_ShowBlockInHand;
extern int  Game_SoundsVolume;
extern int  Game_MusicVolume;
extern bool Game_BreakableLiquids;
extern int  Game_MaxChunkUpdates;
extern bool Game_ScreenshotRequested;

extern float Game_RawHotbarScale, Game_RawChatScale, Game_RawInventoryScale;
float Game_Scale(float value);
float Game_GetHotbarScale(void);
float Game_GetInventoryScale(void);
float Game_GetChatScale(void);

void Game_GetDefaultTexturePack(String* texPack);
void Game_SetDefaultTexturePack(const String* texPack);

bool Game_ChangeTerrainAtlas(Bitmap* atlas);
void Game_SetViewDistance(int distance);
void Game_UserSetViewDistance(int distance);
void Game_UpdateProjection(void);
void Game_Disconnect(const String* title, const String* reason);
void Game_Reset(void);
/* Sets the block in the map at the given coordinates, then updates state associated with the block. */
/* (updating state means recalculating light, redrawing chunk block is in, etc) */
/* NOTE: This does NOT notify the server, use Game_ChangeBlock for that. */
CC_EXPORT void Game_UpdateBlock(int x, int y, int z, BlockID block);
/* Calls Game_UpdateBlock, then sends the block change to the server. */
CC_EXPORT void Game_ChangeBlock(int x, int y, int z, BlockID block);
bool Game_CanPick(BlockID block);
bool Game_UpdateTexture(GfxResourceID* texId, struct Stream* src, const String* file, uint8_t* skinType);
bool Game_ValidateBitmap(const String* file, Bitmap* bmp);
int  Game_CalcRenderType(const String* type);
void Game_SetFpsLimit(enum FpsLimit method);
float Game_CalcLimitMillis(enum FpsLimit method);

void Game_Run(int width, int height, const String* title, struct DisplayDevice* device);
#endif
