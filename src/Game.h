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

/* Width and height of the window. (1 at minimum) */
extern int Game_Width, Game_Height;
/* Total time (in seconds) the game has been running for. */
extern double Game_Time;
/* Number of chunks updated within last second. Resets to 0 after every second. */
extern int Game_ChunkUpdates;
extern struct PickedPos Game_SelectedPos;
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
extern int  Game_Vertices;

extern bool Game_ClassicMode;
extern bool Game_ClassicHacks;
#define Game_PureClassic (Game_ClassicMode && !Game_ClassicHacks)
extern bool Game_AllowCustomBlocks;
extern bool Game_UseCPE;
extern bool Game_AllowServerTextures;

extern bool Game_ViewBobbing;
extern bool Game_BreakableLiquids;
extern bool Game_ScreenshotRequested;
extern bool Game_HideGui;

extern float Game_RawHotbarScale, Game_RawChatScale, Game_RawInventoryScale;
float Game_Scale(float value);
float Game_GetHotbarScale(void);
float Game_GetInventoryScale(void);
float Game_GetChatScale(void);

/* Retrieves the filename of the default texture pack used. */
/* NOTE: Returns default.zip if classic mode or selected pack does not exist. */
void Game_GetDefaultTexturePack(String* texPack);
/* Sets the filename of the default texture pack used. */
void Game_SetDefaultTexturePack(const String* texPack);

/* Attempts to change the terrain atlas. (bitmap containing textures for all blocks) */
bool Game_ChangeTerrainAtlas(Bitmap* atlas);
void Game_SetViewDistance(int distance);
void Game_UserSetViewDistance(int distance);
void Game_UpdateProjection(void);
void Game_Disconnect(const String* title, const String* reason);
void Game_Reset(void);
/* Sets the block in the map at the given coordinates, then updates state associated with the block. */
/* (updating state means recalculating light, redrawing chunk block is in, etc) */
/* NOTE: This does NOT notify the server, use Game_ChangeBlock for that. */
CC_API void Game_UpdateBlock(int x, int y, int z, BlockID block);
/* Calls Game_UpdateBlock, then informs server connection of the block change. */
/* In multiplayer this is sent to the server, in singleplayer just activates physics. */
CC_API void Game_ChangeBlock(int x, int y, int z, BlockID block);
bool Game_CanPick(BlockID block);
bool Game_UpdateTexture(GfxResourceID* texId, struct Stream* src, const String* file, uint8_t* skinType);
/* Checks that the given bitmap can be loaded into a native gfx texture. */
/* (must be power of two size and be <= Gfx_MaxTexWidth/Gfx_MaxHeight) */
bool Game_ValidateBitmap(const String* file, Bitmap* bmp);/* Calculates Game_Width and Game_Height. */
void Game_UpdateClientSize(void);
/* Sets the strategy/method used to limit frames per second. */
void Game_SetFpsLimit(enum FpsLimit method);
/* Returns max time process sleeps for when limiting frames using the given method. */
float Game_CalcLimitMillis(enum FpsLimit method);

/* Runs the main game loop until the window is closed. */
void Game_Run(int width, int height, const String* title);
#endif
