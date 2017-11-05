#ifndef CC_GAME_H
#define CC_GAME_H
#include "Typedefs.h"
#include "Stream.h"
#include "GraphicsEnums.h"
#include "Picking.h"
#include "Options.h"
/* Represents the game.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Width and height of game window */
Int32 Game_Width, Game_Height;
/* Total rendering time(in seconds) elapsed since the client was started. */
Real64 Game_Accumulator;
/* Accumulator for the number of chunk updates performed. Reset every second. */
Int32 Game_ChunkUpdates;
/* Whether the third person camera should have their camera position clipped so as to not intersect blocks.*/
bool Game_CameraClipping;
/* Whether colour clear gfx call should be skipped. */
bool Game_SkipClear;
SkinType Game_DefaultPlayerSkinType;
/* Picked pos instance used for block selection. */
PickedPos Game_SelectedPos;
/* Picked pos instance used for camera clipping. */
PickedPos Game_CameraClipPos;
/* Default index buffer ID. */
Int32 Game_DefaultIb;

/* Whether cobblestone slab to stone brick tiles should be used. */
bool Game_UseCPEBlocks;
/* Account username of the player.*/
String Game_Username;
/* Verification key used for multiplayer, unique for the username and individual server. */
String Game_Mppass;
/* IP address of multiplayer server connected to, null if singleplayer. */
String Game_IPAddress;
/* Port of multiplayer server connected to, 0 if singleplayer. */
Int32 Game_Port;
/* Radius of the sphere the player can see around the position of the current camera. */
Real32 Game_ViewDistance;
/* Maximum view distance player can view up to. */
Real32 Game_MaxViewDistance;
/* 'Real' view distance the player has selected. */
Real32 Game_UserViewDistance;
/* Field of view for the current camera in degrees. */
Int32 Game_Fov;
Int32 Game_DefaultFov, Game_ZoomFov;
/* Strategy used to limit how many frames should be displayed at most each second. */
FpsLimitMethod FpsLimit;
/* Whether lines should be rendered for each axis. */
bool Game_ShowAxisLines;
/* Whether players should animate using simple swinging parallel to their bodies. */
bool Game_SimpleArmsAnim;
/* Whether the arm model should use the classic position. */
bool Game_ClassicArmModel;
/* Whether mouse rotation on the y axis should be inverted. */
bool Game_InvertMouse;
/* Number of vertices used for rendering terrain this frame. */
Int32 Game_Vertices;
/* Model view matrix. */
Matrix Game_View;
/* Projection matrix. */
Matrix Game_Projection;

/* How sensitive the client is to changes in the player's mouse position. */
Int32 Game_MouseSensitivity;
/* Whether tab names autocomplete. */
bool Game_TabAutocomplete;
bool Game_UseClassicGui;
bool Game_UseClassicTabList;
bool Game_UseClassicOptions;
bool Game_ClassicMode;
bool Game_ClassicHacks;
#define Game_PureClassic (Game_ClassicMode && !Game_ClassicHacks)
/* Whether suport for custom blocks is indicated to the server. */
bool Game_AllowCustomBlocks;
/* Whether CPE is used at all. */
bool Game_UseCPE;
/* Whether all texture packs are ignored from the server or not. */
bool Game_AllowServerTextures;
/* Whether advanced/smooth lighting mesh builder is used. */
bool Game_SmoothLighting;
/* Whether chat logging to disc is enabled. */
bool Game_ChatLogging;
/* Whether auto rotation of blocks with special suffixes is enabled. */
bool Game_AutoRotate;
/* Whether camera has smooth rotation. */
bool Game_SmoothCamera;
/* Font name used for text rendering. */
String Game_FontName;
/* Max number of chatlines drawn in chat. */
Int32 Game_ChatLines;
/* Whether clicking on chatlines copies into chat input is enabled. */
bool Game_ClickableChat;
/* Whether the GUI is completely hidden.*/
bool Game_HideGui;
/* Whether FPS status in top left is hidden. */
bool Game_ShowFPS;

/* Whether view bobbing is enabled. */
bool Game_ViewBobbing;
/* Whether block in hand is shown in bottom right. */
bool Game_ShowBlockInHand;
/* Volume of dig and step sounds. 0 means disabled. */
Int32 Game_SoundsVolume;
/* Volume of background music played. 0 means disabled */
Int32 Game_MusicVolume;
/* Whether water/lava can be placed/deleted like normal blocks. */
bool Game_ModifiableLiquids;

Vector3 Game_CurrentCameraPos;
bool Game_ScreenshotRequested;

Real32 Game_RawHotbarScale, Game_RawChatScale, Game_RawInventoryScale;
Real32 Game_Scale(Real32 value);
Real32 Game_GetHotbarScale(void);
Real32 Game_GetInventoryScale(void);
Real32 Game_GetChatScale(void);

void Game_GetDefaultTexturePack(STRING_TRANSIENT String* texPack);
void Game_SetDefaultTexturePack(STRING_PURE String* texPack);
bool Game_GetCursorVisible(void);
void Game_SetCursorVisible(bool visible);

void Game_UpdateProjection(void);
void Game_UpdateBlock(Int32 x, Int32 y, Int32 z, BlockID block);
bool Game_UpdateTexture(GfxResourceID* texId, Stream* src, bool setSkinType);
void Game_SetViewDistance(Real32 distance, bool userDist);
bool Game_CanPick(BlockID block);

/* Performs thread sleeping to limit the FPS. */
static void Game_LimitFPS(void);
/* Frees all resources held by the game. */
void Game_Free(void);
#endif