#ifndef CC_GAME_H
#define CC_GAME_H
#include "String.h"
#include "Picking.h"
#include "Options.h"
/* Represents the game.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

typedef struct Bitmap_ Bitmap;
typedef struct DisplayDevice_ DisplayDevice;
typedef struct Stream_ Stream;

Int32 Game_Width, Game_Height;
/* Total rendering time(in seconds) elapsed since the client was started. */
Real64 Game_Accumulator;
Int32 Game_ChunkUpdates;
bool Game_CameraClipping;
bool Game_SkipClear;
UInt8 Game_DefaultPlayerSkinType;
PickedPos Game_SelectedPos;
PickedPos Game_CameraClipPos;
GfxResourceID Game_DefaultIb;

bool Game_UseCPEBlocks;
extern String Game_Username;
extern String Game_Mppass;
extern String Game_IPAddress;
Int32 Game_Port;
Int32 Game_ViewDistance;
Int32 Game_MaxViewDistance;
Int32 Game_UserViewDistance;
Int32 Game_Fov;
Int32 Game_DefaultFov, Game_ZoomFov;
FpsLimit Game_FpsLimit;
bool Game_ShowAxisLines;
bool Game_SimpleArmsAnim;
bool Game_ClassicArmModel;
bool Game_InvertMouse;
Int32 Game_Vertices;

Int32 Game_MouseSensitivity;
bool Game_TabAutocomplete;
bool Game_UseClassicGui;
bool Game_UseClassicTabList;
bool Game_UseClassicOptions;
bool Game_ClassicMode;
bool Game_ClassicHacks;
#define Game_PureClassic (Game_ClassicMode && !Game_ClassicHacks)
bool Game_AllowCustomBlocks;
bool Game_UseCPE;
bool Game_AllowServerTextures;
bool Game_SmoothLighting;
bool Game_ChatLogging;
bool Game_AutoRotate;
bool Game_SmoothCamera;
extern String Game_FontName;
Int32 Game_ChatLines;
bool Game_ClickableChat;
bool Game_HideGui;
bool Game_ShowFPS;

bool Game_ViewBobbing;
bool Game_ShowBlockInHand;
Int32 Game_SoundsVolume;
Int32 Game_MusicVolume;
bool Game_BreakableLiquids;
Int32 Game_MaxChunkUpdates;

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

bool Game_ChangeTerrainAtlas(Bitmap* atlas);
void Game_SetViewDistance(Int32 distance, bool userDist);
void Game_UpdateProjection(void);
void Game_Disconnect(STRING_PURE String* title, STRING_PURE String* reason);
void Game_UpdateBlock(Int32 x, Int32 y, Int32 z, BlockID block);
bool Game_CanPick(BlockID block);
bool Game_UpdateTexture(GfxResourceID* texId, Stream* src, bool setSkinType);
bool Game_ValidateBitmap(STRING_PURE String* file, Bitmap* bmp);
Int32 Game_CalcRenderType(STRING_PURE String* type);
void Game_SetFpsLimitMethod(FpsLimit method);

void Game_Run(Int32 width, Int32 height, STRING_REF String* title, DisplayDevice* device);
#endif