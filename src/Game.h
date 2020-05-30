#ifndef CC_GAME_H
#define CC_GAME_H
#include "Picking.h"
#include "String.h"
#include "Bitmap.h"
/* Represents the game.
   Copyright 2014-2019 ClassiCube | Licensed under BSD-3
*/

struct DisplayDevice;
struct Stream;

CC_VAR extern struct _GameData {
	/* Width and height of the window. (1 at minimum) */
	int Width, Height;
	/* Total time (in seconds) the game has been running for. */
	double Time;
	/* Number of chunks updated within last second. Resets to 0 after every second. */
	int ChunkUpdates;
} Game;

extern struct RayTracer Game_SelectedPos;
extern cc_bool Game_UseCPEBlocks;

extern String Game_Username;
extern String Game_Mppass;

extern int Game_ViewDistance;
extern int Game_MaxViewDistance;
extern int Game_UserViewDistance;
extern int Game_Fov;
extern int Game_DefaultFov, Game_ZoomFov;

extern int     Game_FpsLimit;
extern cc_bool Game_SimpleArmsAnim;
extern int     Game_Vertices;

extern cc_bool Game_ClassicMode;
extern cc_bool Game_ClassicHacks;
#define Game_PureClassic (Game_ClassicMode && !Game_ClassicHacks)
extern cc_bool Game_AllowCustomBlocks;
extern cc_bool Game_UseCPE;
extern cc_bool Game_AllowServerTextures;

extern cc_bool Game_ViewBobbing;
extern cc_bool Game_BreakableLiquids;
extern cc_bool Game_ScreenshotRequested;
extern cc_bool Game_HideGui;

enum FpsLimitMethod {
	FPS_LIMIT_VSYNC, FPS_LIMIT_30, FPS_LIMIT_60, FPS_LIMIT_120, FPS_LIMIT_144, FPS_LIMIT_NONE, FPS_LIMIT_COUNT
};
extern const char* const FpsLimit_Names[FPS_LIMIT_COUNT];

extern float Game_RawHotbarScale, Game_RawChatScale, Game_RawInventoryScale;
float Game_Scale(float value);
float Game_GetHotbarScale(void);
float Game_GetInventoryScale(void);
float Game_GetChatScale(void);

/* Attempts to change the terrain atlas. (bitmap containing textures for all blocks) */
cc_bool Game_ChangeTerrainAtlas(Bitmap* atlas);
void Game_SetViewDistance(int distance);
void Game_UserSetViewDistance(int distance);
void Game_SetFov(int fov);
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

cc_bool Game_CanPick(BlockID block);
cc_bool Game_UpdateTexture(GfxResourceID* texId, struct Stream* src, const String* file, cc_uint8* skinType);
/* Checks that the given bitmap can be loaded into a native gfx texture. */
/* (must be power of two size and be <= Gfx_MaxTexWidth/Gfx_MaxHeight) */
cc_bool Game_ValidateBitmap(const String* file, Bitmap* bmp);
/* Updates Game_Width and Game_Height. */
void Game_UpdateDimensions(void);
/* Sets the strategy/method used to limit frames per second. */
/* See FPS_LIMIT_ for valid strategies/methods */
void Game_SetFpsLimit(int method);

/* Runs the main game loop until the window is closed. */
void Game_Run(int width, int height, const String* title);
#endif
